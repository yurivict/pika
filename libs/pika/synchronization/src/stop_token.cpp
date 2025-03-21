//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/modules/execution_base.hpp>
#include <pika/modules/thread_support.hpp>
#include <pika/synchronization/mutex.hpp>
#include <pika/synchronization/stop_token.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace pika::detail {

    ///////////////////////////////////////////////////////////////////////////
    void intrusive_ptr_add_ref(stop_state* p)
    {
        p->state_.fetch_add(stop_state::token_ref_increment, std::memory_order_relaxed);
    }

    void intrusive_ptr_release(stop_state* p)
    {
        std::uint64_t old_state =
            p->state_.fetch_sub(stop_state::token_ref_increment, std::memory_order_acq_rel);

        if ((old_state & stop_state::token_ref_mask) == stop_state::token_ref_increment)
        {
            delete p;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void stop_callback_base::add_this_callback(stop_callback_base*& callbacks)
    {
        next_ = callbacks;
        if (next_ != nullptr)
        {
            next_->prev_ = &next_;
        }
        prev_ = &callbacks;
        callbacks = this;
    }

    // returns true if the callback was successfully removed
    bool stop_callback_base::remove_this_callback()
    {
        if (prev_ != nullptr)
        {
            // Still registered, not yet executed: just remove from the list.
            *prev_ = next_;
            if (next_ != nullptr)
            {
                next_->prev_ = prev_;
            }
            return true;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    void stop_state::lock() noexcept
    {
        auto old_state = state_.load(std::memory_order_relaxed);

        auto expected = old_state & ~stop_state::locked_flag;
        while (!state_.compare_exchange_weak(expected, old_state | stop_state::locked_flag,
            std::memory_order_acquire, std::memory_order_relaxed))
        {
            old_state = expected;

            for (std::size_t k = 0; is_locked(old_state); ++k)
            {
                pika::execution::this_thread::detail::yield_k(k, "stop_state::lock");
                old_state = state_.load(std::memory_order_relaxed);
            }

            expected = old_state & ~stop_state::locked_flag;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    bool stop_state::lock_and_request_stop() noexcept
    {
        std::uint64_t old_state = state_.load(std::memory_order_acquire);

        if (stop_requested(old_state))
            return false;

        auto expected = old_state & ~stop_state::locked_flag;
        while (!state_.compare_exchange_weak(expected,
            old_state | stop_state::stop_requested_flag | stop_state::locked_flag,
            std::memory_order_acquire, std::memory_order_relaxed))
        {
            old_state = expected;

            for (std::size_t k = 0; is_locked(old_state); ++k)
            {
                pika::execution::this_thread::detail::yield_k(
                    k, "stop_state::lock_and_request_stop");
                old_state = state_.load(std::memory_order_acquire);

                if (stop_requested(old_state))
                    return false;
            }

            expected = old_state & ~stop_state::locked_flag;
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool stop_state::lock_if_not_stopped(stop_callback_base* cb) noexcept
    {
        std::uint64_t old_state = state_.load(std::memory_order_acquire);

        if (stop_requested(old_state))
        {
            cb->execute();

            cb->callback_finished_executing_.store(true, std::memory_order_release);

            return false;
        }
        else if (!stop_possible(old_state))
        {
            return false;
        }

        auto expected = old_state & ~stop_state::locked_flag;
        while (!state_.compare_exchange_weak(expected, old_state | stop_state::locked_flag,
            std::memory_order_acquire, std::memory_order_relaxed))
        {
            old_state = expected;

            for (std::size_t k = 0; is_locked(old_state); ++k)
            {
                pika::execution::this_thread::detail::yield_k(k, "stop_state::add_callback");
                old_state = state_.load(std::memory_order_acquire);

                if (stop_requested(old_state))
                {
                    cb->execute();

                    cb->callback_finished_executing_.store(true, std::memory_order_release);

                    return false;
                }
                else if (!stop_possible(old_state))
                {
                    return false;
                }
            }

            expected = old_state & ~stop_state::locked_flag;
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    struct scoped_lock_if_not_stopped
    {
        scoped_lock_if_not_stopped(stop_state& state, stop_callback_base* cb) noexcept
          : state_(state)
          , has_lock_(state_.lock_if_not_stopped(cb))
        {
        }
        ~scoped_lock_if_not_stopped()
        {
            if (has_lock_)
                state_.unlock();
        }

        explicit operator bool() const noexcept
        {
            return has_lock_;
        }

        stop_state& state_;
        bool has_lock_;
    };

    bool stop_state::add_callback(stop_callback_base* cb) noexcept
    {
        scoped_lock_if_not_stopped l(*this, cb);
        if (!l)
            return false;

        // Push callback onto callback list
        cb->add_this_callback(callbacks_);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    void stop_state::remove_callback(stop_callback_base* cb) noexcept
    {
        {
            std::lock_guard<stop_state> l(*this);
            if (cb->remove_this_callback())
            {
                return;
            }
        }

        // Callback has either already executed or is executing concurrently
        // on another thread.
        if (signalling_thread_ == pika::threads::detail::get_self_id())
        {
            // Callback executed on this thread or is still currently executing
            // and is unregistering itself from within the callback.
            if (cb->is_removed_ != nullptr)
            {
                // Currently inside the callback, let the request_stop() method
                // know the object is about to be destructed and that it should
                // not try to access the object when the callback returns.
                *cb->is_removed_ = true;
            }
        }
        else
        {
            // Callback is currently executing on another thread,
            // block until it finishes executing.
            pika::util::yield_while(
                [&]() { return !cb->callback_finished_executing_.load(std::memory_order_relaxed); },
                "stop_state::remove_callback");
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    struct scoped_lock_and_request_stop
    {
        scoped_lock_and_request_stop(stop_state& state) noexcept
          : state_(state)
          , has_lock_(state_.lock_and_request_stop())
        {
        }
        ~scoped_lock_and_request_stop()
        {
            if (has_lock_)
                state_.unlock();
        }

        explicit operator bool() const noexcept
        {
            return has_lock_;
        }

        stop_state& state_;
        bool has_lock_;
    };

    bool stop_state::request_stop() noexcept
    {
        // Set the 'stop_requested' signal and acquired the lock.
        scoped_lock_and_request_stop l(*this);
        if (!l)
            return false;    // stop has already been requested.

        PIKA_ASSERT(stop_requested(state_.load(std::memory_order_acquire)));

        signalling_thread_ = pika::threads::detail::get_self_id();

        // invoke registered callbacks
        while (callbacks_ != nullptr)
        {
            // Dequeue the head of the queue
            auto* cb = callbacks_;
            callbacks_ = cb->next_;

            const bool more_callbacks = callbacks_ != nullptr;
            if (more_callbacks)
                callbacks_->prev_ = &callbacks_;

            // Mark this item as removed from the list.
            cb->prev_ = nullptr;

            // Don't hold lock while executing callback so we don't block other
            // threads from unregistering callbacks.
            detail::unlock_guard<stop_state> ul(*this);

            bool is_removed = false;
            cb->is_removed_ = &is_removed;

            cb->execute();

            if (!is_removed)
            {
                cb->is_removed_ = nullptr;
                cb->callback_finished_executing_.store(true, std::memory_order_release);
            }
        }

        return true;
    }
}    // namespace pika::detail
