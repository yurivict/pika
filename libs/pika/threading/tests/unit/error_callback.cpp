//  Copyright (c) 2018 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/init.hpp>
#include <pika/runtime.hpp>
#include <pika/testing.hpp>

#include <atomic>
#include <cstddef>
#include <exception>

std::atomic<std::size_t> count_error_handler(0);

///////////////////////////////////////////////////////////////////////////////
bool on_thread_error(std::size_t, std::exception_ptr const&)
{
    ++count_error_handler;
    return false;
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    PIKA_THROW_EXCEPTION(pika::error::invalid_status, "test", "test");
    return pika::finalize();
}

int main(int argc, char* argv[])
{
    auto on_stop = pika::register_thread_on_error_func(&on_thread_error);
    PIKA_TEST(on_stop.empty());

    bool caught_exception = false;
    try
    {
        pika::init(pika_main, argc, argv);
        PIKA_TEST(false);
    }
    catch (...)
    {
        caught_exception = true;
    }

    PIKA_TEST(caught_exception);
    PIKA_TEST_EQ(count_error_handler, std::size_t(1));

    return 0;
}
