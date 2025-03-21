//  Taken from the Boost.Function library

//  Copyright Douglas Gregor 2001-2003.
//  Copyright 2013 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <pika/functional/function.hpp>
#include <pika/testing.hpp>

#include <cstddef>
#include <stdexcept>

struct stateless_integer_add
{
    int operator()(int x, int y) const
    {
        return x + y;
    }

    void* operator new(std::size_t)
    {
        throw std::runtime_error("Cannot allocate a stateless_integer_add");
    }

    void* operator new(std::size_t, void* p)
    {
        return p;
    }

    void operator delete(void*, void*) noexcept {}

    void operator delete(void*) noexcept {}
};

int main(int, char*[])
{
    pika::util::detail::function<int(int, int)> f;
    f = stateless_integer_add();

    // This test checks that function does not allocate. We should reach this
    // point without an exception having been thrown.
    PIKA_TEST(true);

    return 0;
}
