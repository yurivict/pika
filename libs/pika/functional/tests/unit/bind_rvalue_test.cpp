//  Taken from the Boost.Bind library
//
//  bind_rvalue_test.cpp
//
//  Copyright (c) 2006 Peter Dimov
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if defined(PIKA_MSVC)
# pragma warning(disable : 4786)    // identifier truncated in debug info
# pragma warning(disable : 4710)    // function not inlined
# pragma warning(disable : 4711)    // function selected for automatic inline expansion
# pragma warning(disable : 4514)    // unreferenced inline removed
#endif

#include <pika/functional/bind.hpp>
#include <pika/testing.hpp>

#include <functional>
#include <iostream>

namespace placeholders = std::placeholders;

//

int f(int x)
{
    return x;
}

int main()
{
    PIKA_TEST(pika::util::detail::bind(f, placeholders::_1)(1) == 1);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_2)(1, 2) == 2);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_3)(1, 2, 3) == 3);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_4)(1, 2, 3, 4) == 4);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_5)(1, 2, 3, 4, 5) == 5);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_6)(1, 2, 3, 4, 5, 6) == 6);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_7)(1, 2, 3, 4, 5, 6, 7) == 7);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_8)(1, 2, 3, 4, 5, 6, 7, 8) == 8);

    PIKA_TEST(pika::util::detail::bind(f, placeholders::_9)(1, 2, 3, 4, 5, 6, 7, 8, 9) == 9);

    return 0;
}
