//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2017 Hartmut Kaiser
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#include <pika/execution/detail/execution_parameter_callbacks.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace pika::parallel::execution::detail {
    get_os_thread_count_type& get_get_os_thread_count()
    {
        static get_os_thread_count_type f;
        return f;
    }

    void set_get_os_thread_count(get_os_thread_count_type f)
    {
        get_get_os_thread_count() = f;
    }

    std::size_t get_os_thread_count()
    {
        if (get_get_os_thread_count())
        {
            return get_get_os_thread_count()();
        }
        else
        {
            PIKA_THROW_EXCEPTION(pika::error::invalid_status,
                "pika::parallel::execution::detail::get_os_thread_count",
                "No fallback handler for get_os_thread_count is installed. Please start the "
                "runtime if you haven't done so. If you intended to not use the runtime make sure "
                "you have implemented get_os_thread_count for your executor or install a fallback "
                "handler with pika::parallel::execution::detail::set_get_os_thread_count.");
            return std::size_t(-1);
        }
    }

    get_pu_mask_type& get_get_pu_mask()
    {
        static get_pu_mask_type f;
        return f;
    }

    void set_get_pu_mask(get_pu_mask_type f)
    {
        get_get_pu_mask() = f;
    }

    threads::detail::mask_cref_type get_pu_mask(
        threads::detail::topology& topo, std::size_t thread_num)
    {
        if (get_get_pu_mask())
        {
            return get_get_pu_mask()(topo, thread_num);
        }
        else
        {
            PIKA_THROW_EXCEPTION(pika::error::invalid_status,
                "pika::parallel::execution::detail::get_pu_mask",
                "No fallback handler for get_pu_mask is installed. Please start the runtime if you "
                "haven't done so. If you intended to not use the runtime make sure you have "
                "implemented get_pu_mask for your executor or install a fallback handler with "
                "pika::parallel::execution::detail::set_get_pu_mask.");

            static threads::detail::mask_type mask{};
            return mask;
        }
    }
}    // namespace pika::parallel::execution::detail
