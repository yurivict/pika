//  Copyright (c) 2017-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/modules/itt_notify.hpp>

#include <cstddef>
#include <memory>

namespace pika::detail {
    // By default we don't know anything about the function's name
    template <typename F, typename Enable = void>
    struct get_function_annotation
    {
        static constexpr char const* call(F const& /*f*/) noexcept
        {
            return nullptr;
        }
    };

#if PIKA_HAVE_ITTNOTIFY != 0 && !defined(PIKA_HAVE_APEX)
    template <typename F, typename Enable = void>
    struct get_function_annotation_itt
    {
        static util::itt::string_handle call(F const& f)
        {
            static util::itt::string_handle sh(get_function_annotation<F>::call(f));
            return sh;
        }
    };
#endif
}    // namespace pika::detail
