//  Copyright (c) 2014-2021 Hartmut Kaiser
//  Copyright (c) 2016 Marcin Copik
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>

#include <functional>
#include <type_traits>

namespace pika::traits {
    // new executor framework
    template <typename Parameters, typename Enable = void>
    struct is_executor_parameters;
}    // namespace pika::traits

namespace pika::parallel::execution {
    ///////////////////////////////////////////////////////////////////////////
    // Default sequential executor parameters
    struct sequential_executor_parameters
    {
    };

    // If an executor exposes 'executor_parameter_type' this type is
    // assumed to represent the default parameters for the given executor
    // type.
    template <typename Executor, typename Enable = void>
    struct extract_executor_parameters
    {
        // by default, assume sequential execution
        using type = sequential_executor_parameters;
    };

    template <typename Executor>
    struct extract_executor_parameters<Executor,
        std::void_t<typename Executor::executor_parameters_type>>
    {
        using type = typename Executor::executor_parameters_type;
    };

    ///////////////////////////////////////////////////////////////////////
    // If a parameters type exposes 'has_variable_chunk_size' aliased to
    // std::true_type it is assumed that the number of loop iterations to
    // combine is different for each of the generated chunks.
    template <typename Parameters, typename Enable = void>
    struct extract_has_variable_chunk_size
    {
        // by default, assume equally sized chunks
        using type = std::false_type;
    };

    template <typename Parameters>
    struct extract_has_variable_chunk_size<Parameters,
        std::void_t<typename Parameters::has_variable_chunk_size>>
    {
        using type = typename Parameters::has_variable_chunk_size;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        /// \cond NOINTERNAL
        template <typename T>
        struct is_executor_parameters : std::false_type
        {
        };

        template <>
        struct is_executor_parameters<sequential_executor_parameters> : std::true_type
        {
        };

        template <typename T>
        struct is_executor_parameters<::std::reference_wrapper<T>>
          : pika::traits::is_executor_parameters<T>
        {
        };
        /// \endcond
    }    // namespace detail

    template <typename T>
    struct is_executor_parameters : detail::is_executor_parameters<std::decay_t<T>>
    {
    };

    template <typename T>
    using is_executor_parameters_t = typename is_executor_parameters<T>::type;

    template <typename T>
    inline constexpr bool is_executor_parameters_v = is_executor_parameters<T>::value;
}    // namespace pika::parallel::execution

namespace pika::traits {
    // new executor framework
    template <typename Parameters, typename Enable>
    struct is_executor_parameters
      : parallel::execution::is_executor_parameters<std::decay_t<Parameters>>
    {
    };

    template <typename T>
    using is_executor_parameters_t = typename is_executor_parameters<T>::type;

    template <typename T>
    inline constexpr bool is_executor_parameters_v = is_executor_parameters<T>::value;
}    // namespace pika::traits
