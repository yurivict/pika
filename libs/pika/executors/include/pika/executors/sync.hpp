//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/async_base/sync.hpp>
#include <pika/execution/detail/sync_launch_policy_dispatch.hpp>
#include <pika/execution/executors/execution.hpp>
#include <pika/execution_base/traits/is_executor.hpp>
#include <pika/executors/parallel_executor.hpp>
#include <pika/functional/deferred_call.hpp>

#include <type_traits>
#include <utility>

namespace pika::detail {
    template <typename Func, typename Enable = void>
    struct sync_dispatch_launch_policy_helper;

    template <typename Func>
    struct sync_dispatch_launch_policy_helper<Func, std::enable_if_t<!detail::is_action_v<Func>>>
    {
        template <typename Policy_, typename F, typename... Ts>
        PIKA_FORCEINLINE static auto call(Policy_&& launch_policy, F&& f, Ts&&... ts)
            -> decltype(sync_launch_policy_dispatch<std::decay_t<F>>::call(
                PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...))
        {
            return sync_launch_policy_dispatch<std::decay_t<F>>::call(
                PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
        }
    };

    template <typename Policy>
    struct sync_dispatch<Policy, std::enable_if_t<is_launch_policy_v<Policy>>>
    {
        template <typename Policy_, typename F, typename... Ts>
        PIKA_FORCEINLINE static auto call(Policy_&& launch_policy, F&& f, Ts&&... ts)
            -> decltype(sync_dispatch_launch_policy_helper<std::decay_t<F>>::call(
                PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...))
        {
            return sync_dispatch_launch_policy_helper<std::decay_t<F>>::call(
                PIKA_FORWARD(Policy_, launch_policy), PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
        }
    };

    // Launch the given function or function object synchronously. This exists
    // mostly for symmetry with pika::async.
    template <typename Func, typename Enable>
    struct sync_dispatch
    {
        template <typename F, typename... Ts>
        PIKA_FORCEINLINE static auto call(F&& f, Ts&&... ts)
            -> decltype(parallel::execution::sync_execute(
                execution::parallel_executor(), PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...))
        {
            execution::parallel_executor exec;
            return parallel::execution::sync_execute(
                exec, PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
        }
    };

    // The overload for pika::sync taking an executor simply forwards to the
    // corresponding executor customization point.
    //
    // parallel::execution::executor
    // threads::executor
    template <typename Executor>
    struct sync_dispatch<Executor,
        std::enable_if_t<traits::is_one_way_executor_v<Executor> ||
            traits::is_two_way_executor_v<Executor>>>
    {
        template <typename Executor_, typename F, typename... Ts>
        PIKA_FORCEINLINE static auto call(Executor_&& exec, F&& f, Ts&&... ts)
            -> decltype(parallel::execution::sync_execute(
                PIKA_FORWARD(Executor_, exec), PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...))
        {
            return parallel::execution::sync_execute(
                PIKA_FORWARD(Executor_, exec), PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
        }
    };
}    // namespace pika::detail
