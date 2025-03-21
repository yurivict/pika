//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>

namespace pika::execution {
    ///////////////////////////////////////////////////////////////////////////
    struct simd_policy;

    template <typename Executor, typename Parameters>
    struct simd_policy_shim;

    struct simd_task_policy;

    template <typename Executor, typename Parameters>
    struct simd_task_policy_shim;

    ///////////////////////////////////////////////////////////////////////////
    struct par_simd_policy;

    template <typename Executor, typename Parameters>
    struct par_simd_policy_shim;

    struct par_simd_task_policy;

    template <typename Executor, typename Parameters>
    struct par_simd_task_policy_shim;
}    // namespace pika::execution
