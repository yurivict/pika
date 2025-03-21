//  Copyright (c) 2007-2013 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#include <pika/condition_variable.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/mutex.hpp>
#include <pika/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::int32_t> accumulator;
pika::condition_variable_any result_cv;

void increment(std::int32_t i)
{
    accumulator += i;
    result_cv.notify_one();
}

void increment_with_future(pika::shared_future<std::int32_t> fi)
{
    accumulator += fi.get();
    result_cv.notify_one();
}

///////////////////////////////////////////////////////////////////////////////
struct increment_function_object
{
    void operator()(std::int32_t i) const
    {
        accumulator += i;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct increment_type
{
    void call(std::int32_t i) const
    {
        accumulator += i;
    }
};

auto increment_lambda = [](std::int32_t i) { accumulator += i; };

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    {
        using std::placeholders::_1;

        pika::apply(&increment, 1);
        pika::apply(pika::util::detail::bind(&increment, 1));
        pika::apply(pika::util::detail::bind(&increment, _1), 1);
    }

    {
        pika::lcos::local::promise<std::int32_t> p;
        pika::shared_future<std::int32_t> f = p.get_future();

        using std::placeholders::_1;

        pika::apply(&increment_with_future, f);
        pika::apply(pika::util::detail::bind(&increment_with_future, f));
        pika::apply(pika::util::detail::bind(&increment_with_future, _1), f);

        p.set_value(1);
    }

    {
        using std::placeholders::_1;

        pika::apply(increment, 1);
        pika::apply(pika::util::detail::bind(increment, 1));
        pika::apply(pika::util::detail::bind(increment, _1), 1);
    }

    {
        increment_type inc;

        using std::placeholders::_1;

        pika::apply(&increment_type::call, inc, 1);
        pika::apply(pika::util::detail::bind(&increment_type::call, inc, 1));
        pika::apply(pika::util::detail::bind(&increment_type::call, inc, _1), 1);
    }

    {
        increment_function_object obj;

        using std::placeholders::_1;

        pika::apply(obj, 1);
        pika::apply(pika::util::detail::bind(obj, 1));
        pika::apply(pika::util::detail::bind(obj, _1), 1);
    }

    {
        using std::placeholders::_1;

        pika::apply(increment_lambda, 1);
        pika::apply(pika::util::detail::bind(increment_lambda, 1));
        pika::apply(pika::util::detail::bind(increment_lambda, _1), 1);
    }

    pika::no_mutex result_mutex;
    std::unique_lock<pika::no_mutex> l(result_mutex);
    result_cv.wait_for(l, std::chrono::seconds(1),
        pika::util::detail::bind(std::equal_to<std::int32_t>(), std::ref(accumulator), 18));

    PIKA_TEST_EQ(accumulator.load(), 18);

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    accumulator.store(0);

    // Initialize and run pika
    PIKA_TEST_EQ_MSG(pika::init(pika_main, argc, argv), 0, "pika main exited with non-zero status");

    return 0;
}
