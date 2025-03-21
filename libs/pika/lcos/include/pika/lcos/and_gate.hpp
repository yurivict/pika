//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/functional/bind.hpp>
#include <pika/lcos/conditional_trigger.hpp>
#include <pika/modules/errors.hpp>
#include <pika/synchronization/no_mutex.hpp>
#include <pika/synchronization/spinlock.hpp>
#include <pika/thread_support/assert_owns_lock.hpp>
#include <pika/thread_support/unlock_guard.hpp>

#include <boost/dynamic_bitset.hpp>

#include <cstddef>
#include <list>
#include <mutex>
#include <utility>

namespace pika::lcos::local {
    ///////////////////////////////////////////////////////////////////////////
    template <typename Mutex = pika::spinlock>
    struct base_and_gate
    {
    protected:
        using mutex_type = Mutex;

    private:
        using condition_list_type = std::list<conditional_trigger*>;

    public:
        /// \brief This constructor initializes the base_and_gate object from the
        ///        the number of participants to synchronize the control flow
        ///        with.
        explicit base_and_gate(std::size_t count = 0)
          : received_segments_(count)
          , generation_(0)
        {
        }

        base_and_gate(base_and_gate&& rhs) noexcept
          : mtx_()
          , received_segments_(PIKA_MOVE(rhs.received_segments_))
          , promise_(PIKA_MOVE(rhs.promise_))
          , generation_(rhs.generation_)
          , conditions_(PIKA_MOVE(rhs.conditions_))
        {
            rhs.generation_ = std::size_t(-1);
        }

        base_and_gate& operator=(base_and_gate&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::lock_guard<mutex_type> l(rhs.mtx_);
                mtx_ = mutex_type();
                received_segments_ = PIKA_MOVE(rhs.received_segments_);
                promise_ = PIKA_MOVE(rhs.promise_);
                generation_ = rhs.generation_;
                rhs.generation_ = std::size_t(-1);
                conditions_ = PIKA_MOVE(rhs.conditions_);
            }
            return *this;
        }

    protected:
        bool trigger_conditions(error_code& ec = throws)
        {
            bool triggered = false;
            if (!conditions_.empty())
            {
                error_code rc(throwmode::lightweight);
                for (conditional_trigger* c : conditions_)
                {
                    triggered |= c->set(rc);
                    if (rc && (&ec != &throws))
                        ec = rc;
                }
            }
            else
            {
                if (&ec != &throws)
                    ec = make_success_code();
            }
            return triggered;
        }

    protected:
        /// \brief get a future allowing to wait for the gate to fire
        template <typename OuterLock>
        pika::future<void> get_future(OuterLock& outer_lock, std::size_t count = std::size_t(-1),
            std::size_t* generation_value = nullptr, error_code& ec = pika::throws)
        {
            std::unique_lock<mutex_type> l(mtx_);

            // by default we use as many segments as specified during construction
            if (count == std::size_t(-1))
                count = received_segments_.size();
            PIKA_ASSERT(count != 0);

            init_locked(outer_lock, l, count, ec);
            if (!ec)
            {
                PIKA_ASSERT(generation_ != std::size_t(-1));
                ++generation_;

                // re-check/trigger condition, if needed
                trigger_conditions(ec);
                if (!ec)
                {
                    if (generation_value)
                        *generation_value = generation_;
                    return promise_.get_future(ec);
                }
            }
            return pika::future<void>();
        }

    public:
        pika::future<void> get_future(std::size_t count = std::size_t(-1),
            std::size_t* generation_value = nullptr, error_code& ec = pika::throws)
        {
            no_mutex mtx;
            std::unique_lock<no_mutex> lk(mtx);
            return get_future(lk, count, generation_value, ec);
        }

    protected:
        /// \brief get a shared future allowing to wait for the gate to fire
        template <typename OuterLock>
        pika::shared_future<void>
        get_shared_future(OuterLock& outer_lock, std::size_t count = std::size_t(-1),
            std::size_t* generation_value = nullptr, error_code& ec = pika::throws)
        {
            std::unique_lock<mutex_type> l(mtx_);

            // by default we use as many segments as specified during construction
            if (count == std::size_t(-1))
                count = received_segments_.size();
            PIKA_ASSERT(count != 0);
            PIKA_ASSERT(generation_ != std::size_t(-1));

            if (generation_ == 0)
            {
                init_locked(outer_lock, l, count, ec);
                generation_ = 1;
            }

            if (!ec)
            {
                // re-check/trigger condition, if needed
                trigger_conditions(ec);
                if (!ec)
                {
                    if (generation_value)
                        *generation_value = generation_;
                    return promise_.get_shared_future(ec);
                }
            }
            return pika::future<void>().share();
        }

    public:
        pika::shared_future<void> get_shared_future(std::size_t count = std::size_t(-1),
            std::size_t* generation_value = nullptr, error_code& ec = pika::throws)
        {
            no_mutex mtx;
            std::unique_lock<no_mutex> lk(mtx);
            return get_shared_future(lk, count, generation_value, ec);
        }

    protected:
        /// \brief Set the data which has to go into the segment \a which.
        template <typename OuterLock>
        bool set(std::size_t which, OuterLock outer_lock, error_code& ec = throws)
        {
            PIKA_ASSERT_OWNS_LOCK(outer_lock);

            std::unique_lock<mutex_type> l(mtx_);
            if (which >= received_segments_.size())
            {
                // out of bounds index
                l.unlock();
                outer_lock.unlock();
                PIKA_THROWS_IF(ec, pika::error::bad_parameter, "base_and_gate<>::set",
                    "index is out of range for this base_and_gate");
                return false;
            }
            if (received_segments_.test(which))
            {
                // segment already filled, logic error
                l.unlock();
                outer_lock.unlock();
                PIKA_THROWS_IF(ec, pika::error::bad_parameter, "base_and_gate<>::set",
                    "input with the given index has already been triggered");
                return false;
            }

            if (&ec != &throws)
                ec = make_success_code();

            // set the corresponding bit
            received_segments_.set(which);

            if (received_segments_.count() == received_segments_.size())
            {
                // we have received the last missing segment
                promise<void> p;
                std::swap(p, promise_);
                received_segments_.reset();    // reset data store

                // Unlock the lock to avoid locking problems when triggering
                // the promise
                l.unlock();
                outer_lock.unlock();
                p.set_value();    // fire event

                return true;
            }

            outer_lock.unlock();
            return false;
        }

    public:
        bool set(std::size_t which, error_code& ec = throws)
        {
            no_mutex mtx;
            std::unique_lock<no_mutex> lk(mtx);
            return set(which, PIKA_MOVE(lk), ec);
        }

    protected:
        bool test_condition(std::size_t generation_value)
        {
            return !(generation_value > generation_);
        }

        struct manage_condition
        {
            manage_condition(base_and_gate& gate, conditional_trigger& cond)
              : this_(gate)
            {
                this_.conditions_.push_back(&cond);
                it_ = this_.conditions_.end();
                --it_;    // refer to the newly added element
            }

            ~manage_condition()
            {
                this_.conditions_.erase(it_);
            }

            template <typename Condition>
            pika::future<void> get_future(Condition&& func, error_code& ec = pika::throws)
            {
                return (*it_)->get_future(PIKA_FORWARD(Condition, func), ec);
            }

            base_and_gate& this_;
            condition_list_type::iterator it_;
        };

    public:
        /// \brief Wait for the generational counter to reach the requested
        ///        stage.
        void synchronize(std::size_t generation_value,
            char const* function_name = "base_and_gate<>::synchronize", error_code& ec = throws)
        {
            std::unique_lock<mutex_type> l(mtx_);
            synchronize(generation_value, l, function_name, ec);
        }

    protected:
        template <typename Lock>
        void synchronize(std::size_t generation_value, Lock& l,
            char const* function_name = "base_and_gate<>::synchronize", error_code& ec = throws)
        {
            PIKA_ASSERT_OWNS_LOCK(l);

            if (generation_value < generation_)
            {
                l.unlock();
                PIKA_THROWS_IF(ec, pika::error::invalid_status, function_name,
                    "sequencing error, generational counter too small");
                return;
            }

            // make sure this set operation has not arrived ahead of time
            if (!test_condition(generation_value))
            {
                conditional_trigger c;
                manage_condition cond(*this, c);

                pika::future<void> f = cond.get_future(
                    util::detail::bind(&base_and_gate::test_condition, this, generation_value));

                {
                    pika::detail::unlock_guard<Lock> ul(l);
                    f.get();
                }    // make sure lock gets re-acquired
            }

            if (&ec != &throws)
                ec = make_success_code();
        }

    public:
        std::size_t next_generation()
        {
            std::lock_guard<mutex_type> l(mtx_);
            PIKA_ASSERT(generation_ != std::size_t(-1));
            std::size_t retval = ++generation_;

            trigger_conditions();    // re-check/trigger condition, if needed

            return retval;
        }

        std::size_t generation() const
        {
            std::lock_guard<mutex_type> l(mtx_);
            return generation_;
        }

    protected:
        template <typename OuterLock, typename Lock>
        void init_locked(OuterLock& outer_lock, Lock& l, std::size_t count, error_code& ec = throws)
        {
            if (0 != received_segments_.count())
            {
                // reset happens while part of the slots are filled
                l.unlock();
                outer_lock.unlock();
                PIKA_THROWS_IF(ec, pika::error::bad_parameter, "base_and_gate<>::init",
                    "initializing this base_and_gate while slots are filled");
                return;
            }

            if (received_segments_.size() != count)
                received_segments_.resize(count);    // resize the bitmap
            received_segments_.reset();              // reset all existing bits

            if (&ec != &throws)
                ec = make_success_code();
        }

    private:
        mutable mutex_type mtx_;
        boost::dynamic_bitset<> received_segments_;
        lcos::local::promise<void> promise_;
        std::size_t generation_;
        condition_list_type conditions_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Note: This type is not thread-safe. It has to be protected from
    //       concurrent access by different threads by the code using instances
    //       of this type.
    struct and_gate : public base_and_gate<no_mutex>
    {
    private:
        using base_type = base_and_gate<no_mutex>;

    public:
        and_gate(std::size_t count = 0)
          : base_type(count)
        {
        }

        and_gate(and_gate&& rhs) noexcept
          : base_type(PIKA_MOVE(static_cast<base_type&>(rhs)))
        {
        }

        and_gate& operator=(and_gate&& rhs) noexcept
        {
            if (this != &rhs)
                static_cast<base_type&>(*this) = PIKA_MOVE(rhs);
            return *this;
        }

        template <typename Lock>
        pika::future<void> get_future(Lock& l, std::size_t count = std::size_t(-1),
            std::size_t* generation_value = nullptr, error_code& ec = pika::throws)
        {
            return this->base_type::get_future(l, count, generation_value, ec);
        }

        template <typename Lock>
        pika::shared_future<void> get_shared_future(Lock& l, std::size_t count = std::size_t(-1),
            std::size_t* generation_value = nullptr, error_code& ec = pika::throws)
        {
            return this->base_type::get_shared_future(l, count, generation_value, ec);
        }

        template <typename Lock>
        bool set(std::size_t which, Lock l, error_code& ec = throws)
        {
            return this->base_type::set(which, PIKA_MOVE(l), ec);
        }

        template <typename Lock>
        void synchronize(std::size_t generation_value, Lock& l,
            char const* function_name = "and_gate::synchronize", error_code& ec = throws)
        {
            this->base_type::synchronize(generation_value, l, function_name, ec);
        }
    };
}    // namespace pika::lcos::local
