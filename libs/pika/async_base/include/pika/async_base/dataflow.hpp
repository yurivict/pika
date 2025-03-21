//  Copyright (c) 2007-2018 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/allocator_support/internal_allocator.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace pika::detail {
    template <typename FD, typename Enable = void>
    struct dataflow_dispatch;
}    // namespace pika::detail

///////////////////////////////////////////////////////////////////////////////
namespace pika {
    template <typename F, typename... Ts>
    PIKA_FORCEINLINE auto dataflow(F&& f, Ts&&... ts)
        -> decltype(detail::dataflow_dispatch<std::decay_t<F>>::call(
            pika::detail::internal_allocator<>{}, PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...))
    {
        return detail::dataflow_dispatch<std::decay_t<F>>::call(
            pika::detail::internal_allocator<>{}, PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
    }

    template <typename Allocator, typename F, typename... Ts>
    PIKA_FORCEINLINE auto dataflow_alloc(Allocator const& alloc, F&& f, Ts&&... ts)
        -> decltype(detail::dataflow_dispatch<std::decay_t<F>>::call(
            alloc, PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...))
    {
        return detail::dataflow_dispatch<std::decay_t<F>>::call(
            alloc, PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
    }
}    // namespace pika

// #endif
