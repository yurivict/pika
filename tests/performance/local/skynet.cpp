//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This benchmark provides an equivalent for the benchmarks published at
// https://github.com/atemerev/skynet. It is called the Skynet 1M concurrency
// micro benchmark.
//
// It creates an actor (goroutine, whatever), which spawns 10 new actors, each
// of them spawns 10 more actors, etc. until one million actors are created on
// the final level. Then, each of them returns back its ordinal number (from 0
// to 999999), which are summed on the previous level and sent back upstream,
// until reaching the root actor. (The answer should be 499999500000).

// This code implements two versions of the skynet micro benchmark: a 'normal'
// and a futurized one.

#include <pika/chrono.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>

#include <cstdint>
#include <iostream>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::int64_t skynet(std::int64_t num, std::int64_t size, std::int64_t div)
{
    if (size != 1)
    {
        size /= div;

        std::vector<pika::future<std::int64_t>> results;
        results.reserve(div);

        for (std::int64_t i = 0; i != div; ++i)
        {
            std::int64_t sub_num = num + i * size;
            results.push_back(pika::async(skynet, sub_num, size, div));
        }

        pika::wait_all(results);

        std::int64_t sum = 0;
        for (auto& f : results)
            sum += f.get();
        return sum;
    }
    return num;
}

///////////////////////////////////////////////////////////////////////////////
pika::future<std::int64_t> skynet_f(std::int64_t num, std::int64_t size, std::int64_t div)
{
    if (size != 1)
    {
        size /= div;

        std::vector<pika::future<std::int64_t>> results;
        results.reserve(div);

        for (std::int64_t i = 0; i != div; ++i)
        {
            std::int64_t sub_num = num + i * size;
            results.push_back(pika::async(skynet_f, sub_num, size, div));
        }

        return pika::dataflow(
            [](std::vector<pika::future<std::int64_t>>&& sums) {
                std::int64_t sum = 0;
                for (auto& f : sums)
                    sum += f.get();
                return sum;
            },
            results);
    }
    return pika::make_ready_future(num);
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();

        pika::future<std::int64_t> result = pika::async(skynet, 0, 1000000, 10);
        result.wait();

        auto dur = duration_cast<milliseconds>(high_resolution_clock::now() - start);

        std::cout << "Result 1: " << result.get() << " in " << dur.count() << " ms.\n";
    }

    {
        using namespace std::chrono;
        auto start = high_resolution_clock::now();

        pika::future<std::int64_t> result = pika::async(skynet_f, 0, 1000000, 10);
        result.wait();

        auto dur = duration_cast<milliseconds>(high_resolution_clock::now() - start);

        std::cout << "Result 2: " << result.get() << " in " << dur.count() << " ms.\n";
    }
    return 0;
}

int main(int argc, char* argv[])
{
    return pika::init(pika_main, argc, argv);
}
