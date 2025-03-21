//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2014-2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>

#include <chrono>

namespace pika::chrono {
    using std::chrono::steady_clock;

    class steady_time_point
    {
        using value_type = steady_clock::time_point;

    public:
        steady_time_point(value_type const& abs_time)
          : _abs_time(abs_time)
        {
        }

        template <typename Clock, typename Duration>
        steady_time_point(std::chrono::time_point<Clock, Duration> const& abs_time)
          : _abs_time(std::chrono::time_point_cast<value_type::duration>(
                steady_clock::now() + (abs_time - Clock::now())))
        {
        }

        value_type const& value() const noexcept
        {
            return _abs_time;
        }

    private:
        value_type _abs_time;
    };

    class steady_duration
    {
        using value_type = steady_clock::duration;

    public:
        steady_duration(value_type const& rel_time)
          : _rel_time(rel_time)
        {
        }

        template <typename Rep, typename Period>
        steady_duration(std::chrono::duration<Rep, Period> const& rel_time)
          : _rel_time(std::chrono::duration_cast<value_type>(rel_time))
        {
            if (_rel_time < rel_time)
                ++_rel_time;
        }

        value_type const& value() const noexcept
        {
            return _rel_time;
        }

        steady_clock::time_point from_now() const noexcept
        {
            return steady_clock::now() + _rel_time;
        }

    private:
        value_type _rel_time;
    };
}    // namespace pika::chrono
