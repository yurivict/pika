//  (C) Copyright 2013-2015 Steven R. Brandt
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/functional/bind_front.hpp>
#include <pika/functional/function.hpp>

#include <pika/lcos/composable_guard.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace pika::lcos::local {
    static void run_composable(detail::guard_task* task);

    static void nothing() {}

    namespace detail {
        // A link in the list of tasks attached
        // to a guard
        struct guard_task : detail::debug_object
        {
            guard_atomic next;
            detail::guard_function run;
            bool const single_guard;

            guard_task()
              : next(nullptr)
              , run(nothing)
              , single_guard(true)
            {
            }
            guard_task(bool sg)
              : next(nullptr)
              , run(nothing)
              , single_guard(sg)
            {
            }
        };

        void free(guard_task* task)
        {
            if (task == nullptr)
                return;
            task->check_();
            delete task;
        }
    }    // namespace detail

    void guard_set::sort()
    {
        if (!sorted)
        {
            std::sort(guards.begin(), guards.end());
            (*guards.begin())->check_();
            sorted = true;
        }
    }

    struct stage_data : public detail::debug_object
    {
        guard_set gs;
        detail::guard_function task;
        std::size_t n;
        detail::guard_task** stages;

        stage_data(detail::guard_function task_, std::vector<std::shared_ptr<guard>>& guards)
          : gs()
          , task(PIKA_MOVE(task_))
          , n(guards.size())
          , stages(new detail::guard_task*[n])
        {
            for (std::size_t i = 0; i < n; i++)
            {
                stages[i] = new detail::guard_task(false);
            }
        }

        ~stage_data()
        {
            if (stages == nullptr)
                abort();
            PIKA_ASSERT(n == gs.size());
            delete[] stages;
            stages = nullptr;
        }
    };

    static void run_guarded(guard& g, detail::guard_task* task)
    {
        PIKA_ASSERT(task != nullptr);
        task->check_();
        detail::guard_task* prev = g.task.exchange(task);
        if (prev != nullptr)
        {
            prev->check_();
            detail::guard_task* zero = nullptr;
            if (!prev->next.compare_exchange_strong(zero, task))
            {
                run_composable(task);
                free(prev);
            }
        }
        else
        {
            run_composable(task);
        }
    }

    struct stage_task_cleanup
    {
        stage_data* sd;
        std::size_t n;
        stage_task_cleanup(stage_data* sd_, std::size_t n_)
          : sd(sd_)
          , n(n_)
        {
        }
        ~stage_task_cleanup()
        {
            detail::guard_task* zero = nullptr;
            // The tasks on the other guards had single_task marked,
            // so they haven't had their next field set yet. Setting
            // the next field is necessary if they are going to
            // continue processing.
            for (std::size_t k = 0; k < n; k++)
            {
                detail::guard_task* lt = sd->stages[k];
                lt->check_();
                PIKA_ASSERT(!lt->single_guard);
                zero = nullptr;
                if (!lt->next.compare_exchange_strong(zero, lt))
                {
                    PIKA_ASSERT(zero != lt);
                    run_composable(zero);
                    free(lt);
                }
            }
            delete sd;
        }
    };

    static void stage_task(stage_data* sd, std::size_t i, std::size_t n)
    {
        // if this is the last task in the set...
        if (i + 1 == n)
        {
            stage_task_cleanup stc(sd, n);
            sd->task();
        }
        else
        {
            std::size_t k = i + 1;
            detail::guard_task* stage = sd->stages[k];
            stage->run = util::detail::bind_front(stage_task, sd, k, n);
            PIKA_ASSERT(!stage->single_guard);
            run_guarded(*sd->gs.get(k), stage);
        }
    }

    void run_guarded(guard_set& guards, detail::guard_function task)
    {
        std::size_t n = guards.guards.size();
        if (n == 0)
        {
            task();
            return;
        }
        else if (n == 1)
        {
            run_guarded(*guards.guards[0], PIKA_MOVE(task));
            guards.check_();
            return;
        }
        guards.sort();
        stage_data* sd = new stage_data(PIKA_MOVE(task), guards.guards);
        int k = 0;
        sd->stages[k]->run = util::detail::bind_front(stage_task, sd, k, n);
        sd->gs = guards;
        detail::guard_task* stage = sd->stages[k];    //-V108
        run_guarded(*sd->gs.get(k), stage);           //-V106
    }

    void run_guarded(guard& guard, detail::guard_function task)
    {
        detail::guard_task* tptr = new detail::guard_task();
        tptr->run = PIKA_MOVE(task);
        run_guarded(guard, tptr);
    }

    // This class exists so that a destructor is
    // used to perform cleanup. By using a destructor
    // we ensure the code works even if exceptions are
    // thrown.
    struct run_composable_cleanup
    {
        detail::guard_task* task;
        run_composable_cleanup(detail::guard_task* task_)
          : task(task_)
        {
        }
        ~run_composable_cleanup()
        {
            detail::guard_task* zero = nullptr;
            // If single_guard is false, then this is one of the
            // setup tasks for a multi-guarded task. By not setting
            // the next field, we halt processing on items queued
            // to this guard.
            PIKA_ASSERT(task != nullptr);
            task->check_();
            if (!task->next.compare_exchange_strong(zero, task))
            {
                PIKA_ASSERT(zero != nullptr);
                run_composable(zero);
                free(task);
            }
        }
    };

    namespace detail {
        struct empty_helper
        {
            static guard_task*& get_empty_guard_task()
            {
                static guard_task* empty = new guard_task;
                return empty;
            }

            empty_helper() = default;
            ~empty_helper()
            {
                auto& empty = get_empty_guard_task();
                delete empty;
                empty = nullptr;
            }
        };

        empty_helper empty_helper_{};
    }    // namespace detail

    using pika::lcos::local::detail::guard_task;
    static guard_task empty;

    static void run_composable(detail::guard_task* task)
    {
        if (task == &empty)
            return;
        PIKA_ASSERT(task != nullptr);
        task->check_();
        if (task->single_guard)
        {
            run_composable_cleanup rcc(task);
            task->run();
        }
        else
        {
            task->run();
            // Note that by this point in the execution
            // the task data structure has probably
            // been deleted.
        }
    }

    guard::~guard()
    {
        guard_task* zero = nullptr;
        guard_task* current = task.load();
        if (current == nullptr)
            return;
        if (!current->next.compare_exchange_strong(zero, &empty))
        {
            free(zero);
        }
    }
}    // namespace pika::lcos::local
