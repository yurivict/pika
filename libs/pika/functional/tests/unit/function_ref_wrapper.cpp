//  Taken from the Boost.Function library

//  Copyright (C) 2001-2003 Douglas Gregor
//  Copyright 2013 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org/

#include <pika/functional/function.hpp>
#include <pika/testing.hpp>

#include <functional>

struct stateful_type
{
    int operator()(int x) const
    {
        return x;
    }
};

int main()
{
    stateful_type a_function_object;
    pika::util::detail::function<int(int)> f;

    f = std::ref(a_function_object);
    PIKA_TEST_EQ(f(42), 42);
    pika::util::detail::function<int(int)> f2(f);
    PIKA_TEST_EQ(f2(42), 42);

    return 0;
}
