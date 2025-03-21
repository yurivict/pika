//  Copyright (c) 2019 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is inspired by https://github.com/aprell/tasking-2.0

#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/modules/timing.hpp>
#include <pika/synchronization/channel_mpsc.hpp>
#include <pika/thread.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
struct data
{
    data() = default;

    explicit data(int d)
    {
        data_[0] = d;
    }

    int data_[8];
};

#if PIKA_DEBUG
constexpr int NUM_TESTS = 1000000;
#else
constexpr int NUM_TESTS = 100000000;
#endif

///////////////////////////////////////////////////////////////////////////////
inline data channel_get(pika::experimental::channel_mpsc<data> const& c)
{
    data result;
    while (!c.get(&result))
    {
        pika::this_thread::yield();
    }
    return result;
}

inline void channel_set(pika::experimental::channel_mpsc<data>& c, data&& val)
{
    while (!c.set(std::move(val)))    // NOLINT
    {
        pika::this_thread::yield();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Produce
double thread_func_0(pika::experimental::channel_mpsc<data>& c)
{
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i != NUM_TESTS; ++i)
    {
        channel_set(c, data{i});
    }

    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration<double>(end - start).count();
}

// Consume
double thread_func_1(pika::experimental::channel_mpsc<data>& c)
{
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i != NUM_TESTS; ++i)
    {
        data d = channel_get(c);
        if (d.data_[0] != i)
        {
            std::cout << "Error!\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration<double>(end - start).count();
}

int pika_main()
{
    pika::experimental::channel_mpsc<data> c(10000);

    pika::future<double> producer = pika::async(thread_func_0, std::ref(c));
    pika::future<double> consumer = pika::async(thread_func_1, std::ref(c));

    auto producer_time = producer.get();
    std::cout << "Producer throughput: " << (NUM_TESTS / producer_time) << " [op/s] ("
              << (producer_time / NUM_TESTS) << " [s/op])\n";

    auto consumer_time = consumer.get();
    std::cout << "Consumer throughput: " << (NUM_TESTS / consumer_time) << " [op/s] ("
              << (consumer_time / NUM_TESTS) << " [s/op])\n";

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    return pika::init(pika_main, argc, argv);
}
