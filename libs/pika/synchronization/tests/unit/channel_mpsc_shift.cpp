//  Copyright (c) 2019 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is inspired by https://github.com/aprell/tasking-2.0

#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/synchronization/channel_mpsc.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

constexpr int NUM_WORKERS = 1000;

///////////////////////////////////////////////////////////////////////////////
template <typename T>
inline T channel_get(pika::experimental::channel_mpsc<T> const& c)
{
    T result;
    while (!c.get(&result))
    {
        pika::this_thread::yield();
    }
    return result;
}

template <typename T>
inline void channel_set(pika::experimental::channel_mpsc<T>& c, T val)
{
    while (!c.set(std::move(val)))    // NOLINT
    {
        pika::this_thread::yield();
    }
}

///////////////////////////////////////////////////////////////////////////////
int thread_func(int i, pika::experimental::channel_mpsc<int>& channel,
    pika::experimental::channel_mpsc<int>& next)
{
    channel_set(channel, i);
    return channel_get(next);
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    std::vector<pika::experimental::channel_mpsc<int>> channels;
    channels.reserve(NUM_WORKERS);

    std::vector<pika::future<int>> workers;
    workers.reserve(NUM_WORKERS);

    for (int i = 0; i != NUM_WORKERS; ++i)
    {
        channels.emplace_back(std::size_t(1));
    }

    for (int i = 0; i != NUM_WORKERS; ++i)
    {
        workers.push_back(pika::async(
            &thread_func, i, std::ref(channels[i]), std::ref(channels[(i + 1) % NUM_WORKERS])));
    }

    pika::wait_all(workers);

    for (int i = 0; i != NUM_WORKERS; ++i)
    {
        PIKA_TEST_EQ((i + 1) % NUM_WORKERS, workers[i].get());
    }

    pika::finalize();
    return 0;
}

int main(int argc, char* argv[])
{
    return pika::init(pika_main, argc, argv);
}
