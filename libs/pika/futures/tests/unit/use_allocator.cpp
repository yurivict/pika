//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (C) 2011 Vicente J. Botet Escriba
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/future.hpp>

#include <memory>

#include "test_allocator.hpp"

int main()
{
    // pika::lcos::local::promise
    static_assert(std::uses_allocator<pika::lcos::local::promise<int>, test_allocator<int>>::value,
        "std::uses_allocator<local::promise<int>, test_allocator<int> >::value");
    static_assert(std::uses_allocator<pika::lcos::local::promise<int&>, test_allocator<int>>::value,
        "std::uses_allocator<local::promise<int&>, test_allocator<int> >::value");
    static_assert(
        std::uses_allocator<pika::lcos::local::promise<void>, test_allocator<void>>::value,
        "std::uses_allocator<local::promise<void>, test_allocator<void> >::value");

    // pika::lcos::local::packaged_task
    static_assert(
        std::uses_allocator<pika::lcos::local::packaged_task<int()>, test_allocator<int>>::value,
        "std::uses_allocator<local::packaged_task<int()>, test_allocator<int> >::value");

    return 0;
}
