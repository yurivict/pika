//  Copyright (c) 2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This must fail compiling

#include <pika/synchronization/spinlock.hpp>

#include <utility>

///////////////////////////////////////////////////////////////////////////////
int main()
{
    using pika::spinlock;
    spinlock m1, m2(std::move(m1));
    return 0;
}
