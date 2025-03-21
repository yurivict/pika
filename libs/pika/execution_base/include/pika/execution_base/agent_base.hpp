//  Copyright (c) 2019 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/execution_base/context_base.hpp>
#include <pika/timing/steady_clock.hpp>

#include <cstddef>
#include <string>

namespace pika::execution::detail {
    struct agent_base
    {
        virtual ~agent_base() = default;

        virtual std::string description() const = 0;

        virtual context_base const& context() const = 0;

        virtual void yield(char const* desc) = 0;
        virtual void yield_k(std::size_t k, char const* desc) = 0;
        virtual void suspend(char const* desc) = 0;
        virtual void resume(char const* desc) = 0;
        virtual void abort(char const* desc) = 0;
        virtual void sleep_for(
            pika::chrono::steady_duration const& sleep_duration, char const* desc) = 0;
        virtual void sleep_until(
            pika::chrono::steady_time_point const& sleep_time, char const* desc) = 0;
    };
}    // namespace pika::execution::detail
