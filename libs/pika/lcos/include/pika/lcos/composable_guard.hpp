//  (C) Copyright 2013-2015 Steven R. Brandt
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  How do guards work:
//  Pythonesque pseudocode
//
//  class guard:
//    task # an atomic pointer to a guard_task
//
//  class guard_task:
//    run # a function pointer of some kind
//    next # an atomic pointer to another guard task
//
//  def run_guarded(g,func):
//    n = new guard_task
//    n.run = func
//    t = g.task.exchange(n)
//    if t == nullptr:
//      run_task(n)
//    else:
//      zero = nullptr
//      if t.next.compare_exchange_strong(zero,n):
//        pass
//      else:
//        run_task(n)
//        delete t
//
//  def run_task(t):
//    t.run() // call the task
//    zero = nullptr
//    if t.next.compare_exchange_strong(zero,t):
//      pass
//    else:
//      run_task(zero)
//      delete t
//
// Consider cases. Thread A, B, and C on guard g.
// Case 1:
// Thread A runs on guard g, gets t == nullptr and runs to completion.
// Thread B starts, gets t != null, compare_exchange_strong fails,
//  it runs to completion and deletes t
// Thread C starts, gets t != null, compare_exchange_strong fails,
//  it runs to completion and deletes t
//
// Case 2:
// Thread A runs on guard g, gets t == nullptr,
//  but before it completes, thread B starts.
// Thread B runs on guard g, gets t != nullptr,
//  compare_exchange_strong succeeds. It does nothing further.
// Thread A resumes and finishes, compare_exchange_strong fails,
//  it runs B's task to completion.
// Thread C starts, gets t != null, compare_exchange_strong fails,
//  it runs to completion and deletes t
//
// Case 3:
// Thread A runs on guard g, gets t == nullptr,
//  but before it completes, thread B starts.
// Thread B runs on guard g, gets t != nullptr,
//  compare_exchange_strong succeeds, It does nothing further.
// Thread C runs on guard g, gets t != nullptr,
//  compare_exchange_strong succeeds, It does nothing further.
// Thread A resumes and finishes, compare_exchange_strong fails,
//  it runs B's task to completion.
// Thread B does compare_exchange_strong fails,
//  it runs C's task to completion.
//
//  def destructor guard:
//    t = g.load()
//    if t == nullptr:
//      pass
//    else:
//      zero = nullptr
//      if t.next.compare_exchange_strong(zero,empty):
//        pass
//      else:
//        delete t

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/functional/deferred_call.hpp>
#include <pika/functional/unique_function.hpp>
#include <pika/futures/packaged_task.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace pika::lcos::local {
    namespace detail {
        struct debug_object
        {
#ifdef PIKA_DEBUG
            static constexpr int debug_magic = 0x2cab;

            int magic;

            debug_object()
              : magic(debug_magic)
            {
            }

            ~debug_object()
            {
                check_();
                magic = ~debug_magic;
            }

            void check_()
            {
                PIKA_ASSERT(magic != ~debug_magic);
                PIKA_ASSERT(magic == debug_magic);
            }
#else
            void check_() {}
#endif
        };

        struct guard_task;

        using guard_atomic = std::atomic<guard_task*>;

        PIKA_EXPORT void free(guard_task* task);

        using guard_function = util::detail::unique_function<void()>;
    }    // namespace detail

    class guard : public detail::debug_object
    {
    public:
        detail::guard_atomic task;

        guard()
          : task(nullptr)
        {
        }
        PIKA_EXPORT ~guard();
    };

    class guard_set : public detail::debug_object
    {
        std::vector<std::shared_ptr<guard>> guards;
        // the guards need to be sorted, but we don't
        // want to sort them more often than necessary
        bool sorted;

        void sort();

    public:
        guard_set()
          : guards()
          , sorted(true)
        {
        }
        ~guard_set() {}

        std::shared_ptr<guard> get(std::size_t i)
        {
            return guards[i];
        }

        void add(std::shared_ptr<guard> const& guard_ptr)
        {
            PIKA_ASSERT(guard_ptr.get() != nullptr);
            guards.push_back(guard_ptr);
            sorted = false;
        }

        std::size_t size()
        {
            return guards.size();
        }

        friend PIKA_EXPORT void run_guarded(guard_set& guards, detail::guard_function task);
    };

    /// Conceptually, a guard acts like a mutex on an asynchronous task. The
    /// mutex is locked before the task runs, and unlocked afterwards.
    PIKA_EXPORT void run_guarded(guard& guard, detail::guard_function task);

    template <typename F, typename... Args>
    void run_guarded(guard& guard, F&& f, Args&&... args)
    {
        return run_guarded(guard,
            detail::guard_function(
                util::detail::deferred_call(PIKA_FORWARD(F, f), PIKA_FORWARD(Args, args)...)));
    }

    /// Conceptually, a guard_set acts like a set of mutexes on an asynchronous task.
    /// The mutexes are locked before the task runs, and unlocked afterwards.
    PIKA_EXPORT void run_guarded(guard_set& guards, detail::guard_function task);

    template <typename F, typename... Args>
    void run_guarded(guard_set& guards, F&& f, Args&&... args)
    {
        return run_guarded(guards,
            detail::guard_function(
                util::detail::deferred_call(PIKA_FORWARD(F, f), PIKA_FORWARD(Args, args)...)));
    }
}    // namespace pika::lcos::local
