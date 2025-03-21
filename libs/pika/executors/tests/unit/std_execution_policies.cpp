//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/execution.hpp>
#include <pika/init.hpp>
#include <pika/testing.hpp>

///////////////////////////////////////////////////////////////////////////////
void static_checks()
{
    static_assert(pika::is_execution_policy<std::execution::sequenced_policy>::value,
        "pika::is_execution_policy<std::execution::sequenced_policy>::value");
    static_assert(pika::is_execution_policy<std::execution::parallel_policy>::value,
        "pika::is_execution_policy<std::execution::parallel_policy>::value");
    static_assert(pika::is_execution_policy<std::execution::parallel_unsequenced_policy>::value,
        "pika::is_execution_policy<std::execution::parallel_unsequenced_policy>::value");

    static_assert(pika::is_sequenced_execution_policy<std::execution::sequenced_policy>::value,
        "pika::is_sequenced_execution_policy<std::execution::sequenced_policy>::value");
    static_assert(!pika::is_sequenced_execution_policy<std::execution::parallel_policy>::value,
        "!pika::is_sequenced_execution_policy<std::execution::parallel_policy>::value");
    static_assert(
        !pika::is_sequenced_execution_policy<std::execution::parallel_unsequenced_policy>::value,
        "!pika::is_sequenced_execution_policy<std::execution::parallel_unsequenced_policy>::value");

    static_assert(!pika::is_parallel_execution_policy<std::execution::sequenced_policy>::value,
        "!pika::is_sequenced_execution_policy<std::execution::sequenced_policy>::value");
    static_assert(pika::is_parallel_execution_policy<std::execution::parallel_policy>::value,
        "pika::is_parallel_execution_policy<std::execution::parallel_policy>::value");
    static_assert(
        pika::is_parallel_execution_policy<std::execution::parallel_unsequenced_policy>::value,
        "pika::is_parallel_execution_policy<std::execution::parallel_unsequenced_policy>::value");

#if defined(PIKA_HAVE_CXX20_STD_EXECUTION_POLICIES)
    static_assert(pika::is_execution_policy<std::execution::unsequenced_policy>::value,
        "pika::is_execution_policy<std::execution::unsequenced_policy>::value");
    static_assert(pika::is_sequenced_execution_policy<std::execution::unsequenced_policy>::value,
        "pika::is_sequenced_execution_policy<std::execution::unsequenced_policy>::value");
    static_assert(!pika::is_parallel_execution_policy<std::execution::unsequenced_policy>::value,
        "!pika::is_parallel_execution_policy<std::execution::unsequenced_policy>::value");
#endif
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    static_checks();

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run pika
    PIKA_TEST_EQ_MSG(pika::init(pika_main, argc, argv), 0, "pika main exited with non-zero status");

    return 0;
}
