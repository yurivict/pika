//  Taken from the Boost.Bind library
//
//  bind_dm3_test.cpp - data members (regression 1.31 - 1.32)
//
//  Copyright (c) 2005 Peter Dimov
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

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
#include <utility>

namespace placeholders = std::placeholders;

int main()
{
    using pair_type = std::pair<int, int>;

    pair_type pair(10, 20);

    int const& x = pika::util::detail::bind(&pair_type::first, placeholders::_1)(pair);

    PIKA_TEST_EQ(&pair.first, &x);

    return 0;
}
