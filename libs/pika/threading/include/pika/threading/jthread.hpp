//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/functional/detail/invoke.hpp>
#include <pika/synchronization/stop_token.hpp>
#include <pika/threading/thread.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace pika {

    class jthread
    {
    private:
        template <typename F, typename... Ts>
        static void invoke(std::false_type, F&& f, stop_token&& /* st */, Ts&&... ts)
        {
            // started thread does not expect a stop token:
            PIKA_INVOKE(PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        static void invoke(std::true_type, F&& f, stop_token&& st, Ts&&... ts)
        {
            // pass the stop_token as first argument to the started thread:
            PIKA_INVOKE(PIKA_FORWARD(F, f), PIKA_MOVE(st), PIKA_FORWARD(Ts, ts)...);
        }

    public:
        // types
        using id = thread::id;
        using native_handle_type = thread::native_handle_type;

        // 32.4.3.1, constructors, move, and assignment

        // Effects: Constructs a jthread object that does not represent a
        //      thread of execution
        //
        // Ensures: get_id() == id() is true and ssource_.stop_possible() is
        //      false.
        jthread() noexcept
          : ssource_{nostopstate}
        {
        }

        // Requires: F and each T in Ts meet the Cpp17MoveConstructible
        //      requirements. Either
        //
        //      INVOKE(decay-copy(PIKA_FORWARD(F, f)), get_stop_token(),
        //             decay-copy(PIKA_FORWARD(Ts, ts))...)
        //
        //      is a valid expression or
        //
        //      INVOKE(decay-copy(PIKA_FORWARD(F, f)),
        //             decay-copy(PIKA_FORWARD(Ts, ts))...)
        //
        //      is a valid expression.
        //
        // Constraints: remove_cvref_t<F> is not the same type as jthread.
        //
        // Effects: Initializes ssource_ and constructs an object of type
        //      jthread. The new thread of execution executes
        //
        //      INVOKE(decay-copy(PIKA_FORWARD(F, f)), get_stop_token(),
        //             decay-copy(PIKA_FORWARD(Ts, ts))...)
        //
        //      if that expression is well-formed, otherwise
        //
        //      INVOKE(decay-copy(PIKA_FORWARD(F, f)),
        //             decay-copy(PIKA_FORWARD(Ts, ts))...)
        //
        //      with the calls to decay-copy being evaluated in the
        //      constructing thread. Any return value from this invocation
        //      is ignored. If the INVOKE expression exits via an exception,
        //      terminate is called.
        //
        // Synchronization: The completion of the invocation of the
        //      constructor synchronizes with the beginning of the invocation
        //      of the copy of f.
        //
        // Ensures: get_id() != id() is true and ssource_.stop_possible() is
        //      true and *this represents the newly started thread.
        //
        // Throws: system_error if unable to start the new thread.
        //
        // Error conditions:
        //      resource_unavailable_try_again - the system lacked the
        //          necessary resources to create another thread, or the
        //          system-imposed limit on the number of threads in a process
        //          would be exceeded.
        //
        template <typename F, typename... Ts,
            typename Enable =
                typename std::enable_if<!std::is_same<std::decay_t<F>, jthread>::value>::type>
        explicit jthread(F&& f, Ts&&... ts)
          : ssource_{}    // initialize stop_source
          , thread_{
                // lambda called in the thread
                [](stop_token st, F&& f, Ts&&... ts) -> void {
                    // perform tasks of the thread
                    using use_stop_token = typename std::is_invocable<F, stop_token, Ts...>::type;

                    jthread::invoke(use_stop_token{}, PIKA_FORWARD(F, f), PIKA_MOVE(st),
                        PIKA_FORWARD(Ts, ts)...);
                },
                // not captured due to possible races if immediately set
                ssource_.get_token(), PIKA_FORWARD(F, f),    // pass callable
                PIKA_FORWARD(Ts, ts)...                      // pass arguments for callable
            }
        {
        }

        // Effects: If joinable() is true, calls request_stop() and then join().
        ~jthread()
        {
            if (joinable())
            {
                // if not joined/detached, signal stop and wait for end:
                request_stop();
                join();
            }
        }

        jthread(jthread const&) = delete;

        // Effects: Constructs an object of type jthread from x, and sets
        //      x to a default constructed state.
        //
        // Ensures: x.get_id() == id() and get_id() returns the value of
        //      x.get_id() prior to the start of construction. ssource_ has
        //      the value of x.ssource_ prior to the start of construction
        //      and x.ssource_.stop_possible() is false.
        //
        jthread(jthread&& x) noexcept = default;

        jthread& operator=(jthread const&) = delete;

        // Effects: If joinable() is true, calls request_stop() and then join().
        //      Assigns the state of x to *this and sets x to a default
        //      constructed state.
        //
        // Ensures: x.get_id() == id() and get_id() returns the value of
        //      x.get_id() prior to the assignment. ssource_ has the value of
        //      x.ssource_ prior to the assignment and x.ssource_.stop_possible()
        //      is false.
        //
        // Returns: *this.
        jthread& operator=(jthread&&) noexcept = default;

        // 32.4.3.2, members

        // Effects: Exchanges the values of *this and x.
        void swap(jthread& t) noexcept
        {
            std::swap(ssource_, t.ssource_);
            std::swap(thread_, t.thread_);
        }

        // Returns: get_id() != id().
        [[nodiscard]] bool joinable() const noexcept
        {
            return thread_.joinable();
        }

        // Effects: Blocks until the thread represented by *this has
        //      completed.
        //
        // Synchronization: The completion of the thread represented
        //      by *this synchronizes with the corresponding successful
        //      join() return.
        //
        // Ensures: The thread represented by *this has completed.
        //      get_id() == id().
        //
        // Throws: system_error when an exception is required.
        //
        // Error conditions:
        //      - resource_deadlock_would_occur - if deadlock is detected
        //          or get_id() == thisthread_::get_id().
        //      - no_such_process - if the thread is not valid.
        //      - invalid_argument - if the thread is not joinable.
        void join()
        {
            thread_.join();
        }

        // Effects: The thread represented by *this continues execution
        //      without the calling thread blocking. When detach() returns,
        //      *this no longer represents the possibly continuing thread
        //      of execution. When the thread previously represented by
        //      *this ends execution, the implementation shall release
        //      any owned resources.
        //
        // Ensures: get_id() == id().
        //
        // Throws: system_error when an exception is required
        //
        // Error conditions:
        //      - no_such_process - if the thread is not valid.
        //      - invalid_argument - if the thread is not joinable.
        void detach()
        {
            thread_.detach();
        }

        // Returns: A default constructed id object if *this does not
        //      represent a thread, otherwise thisthread_::get_id() for
        //      the thread of execution represented by *this.
        [[nodiscard]] id get_id() const noexcept
        {
            return thread_.get_id();
        }

        // The presence of native_handle() and its semantic is
        //      implementation-defined.
        [[nodiscard]] native_handle_type native_handle()
        {
            return thread_.native_handle();
        }

        // 32.4.3.2, stop token handling

        // Effects: Equivalent to: return ssource_;
        [[nodiscard]] stop_source get_stop_source() noexcept
        {
            return ssource_;
        }

        // Effects: Equivalent to: return ssource_.get_token();
        [[nodiscard]] stop_token get_stop_token() const noexcept
        {
            return ssource_.get_token();
        }

        // Effects: Equivalent to: return ssource_.request_stop();
        bool request_stop() noexcept
        {
            return ssource_.request_stop();
        }

        // 32.4.3.5, static members

        // Returns: thread::hardware_concurrency().
        [[nodiscard]] static unsigned int hardware_concurrency()
        {
            return pika::threads::detail::hardware_concurrency();
        }

    private:
        stop_source ssource_;      // stop_source for started thread
        pika::thread thread_{};    // started thread (if any)
    };

    // 32.4.3.4, specialized algorithms

    // Effects: Equivalent to: x.swap(y).
    inline void swap(jthread& lhs, jthread& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}    // namespace pika
