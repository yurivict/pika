//  Copyright (c) 2007-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/async_base/async.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace pika {
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        // dispatch point used for async<Action> implementations
        template <typename Action, typename Func, typename Enable = void>
        struct async_action_dispatch;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename F, typename... Ts>
    PIKA_FORCEINLINE auto async(F&& f, Ts&&... ts)
        -> decltype(detail::async_action_dispatch<Action, std::decay_t<F>>::call(
            PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...));
}    // namespace pika
