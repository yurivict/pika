//  Copyright (c) 2013 Thomas Heller
//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/execution.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/pack_traversal/unwrap.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using pika::program_options::options_description;
using pika::program_options::value;
using pika::program_options::variables_map;

using pika::dataflow;
using pika::util::detail::bind;

using pika::async;
using pika::future;
using pika::shared_future;

using pika::make_ready_future;

using pika::finalize;
using pika::init;

using pika::unwrapping;

///////////////////////////////////////////////////////////////////////////////

std::atomic<std::uint32_t> void_f_count;
std::atomic<std::uint32_t> int_f_count;

void void_f()
{
    ++void_f_count;
}
int int_f()
{
    ++int_f_count;
    return 42;
}

std::atomic<std::uint32_t> void_f1_count;
std::atomic<std::uint32_t> int_f1_count;

void void_f1(int)
{
    ++void_f1_count;
}
int int_f1(int i)
{
    ++int_f1_count;
    return i + 42;
}

std::atomic<std::uint32_t> int_f2_count;
int int_f2(int l, int r)
{
    ++int_f2_count;
    return l + r;
}

std::atomic<std::uint32_t> int_f_vector_count;

int int_f_vector(std::vector<int> const& vf)
{
    int sum = 0;
    for (int f : vf)
    {
        sum += f;
    }
    return sum;
}

template <typename Executor>
void function_pointers(Executor& exec)
{
    void_f_count.store(0);
    int_f_count.store(0);
    void_f1_count.store(0);
    int_f1_count.store(0);
    int_f2_count.store(0);

    future<void> f1 = dataflow(exec, unwrapping(&void_f1), async(&int_f));
    future<int> f2 = dataflow(
        exec, unwrapping(&int_f1), dataflow(exec, unwrapping(&int_f1), make_ready_future(42)));
    future<int> f3 = dataflow(exec, unwrapping(&int_f2),
        dataflow(exec, unwrapping(&int_f1), make_ready_future(42)),
        dataflow(exec, unwrapping(&int_f1), make_ready_future(37)));

    int_f_vector_count.store(0);
    std::vector<future<int>> vf;
    for (std::size_t i = 0; i < 10; ++i)
    {
        vf.push_back(dataflow(exec, unwrapping(&int_f1), make_ready_future(42)));
    }
    future<int> f4 = dataflow(exec, unwrapping(&int_f_vector), std::move(vf));

    future<int> f5 = dataflow(exec, unwrapping(&int_f1),
        dataflow(exec, unwrapping(&int_f1), make_ready_future(42)),
        dataflow(exec, unwrapping(&void_f), make_ready_future()));

    f1.wait();
    PIKA_TEST_EQ(f2.get(), 126);
    PIKA_TEST_EQ(f3.get(), 163);
    PIKA_TEST_EQ(f4.get(), 10 * 84);
    PIKA_TEST_EQ(f5.get(), 126);
    PIKA_TEST_EQ(void_f_count, 1u);
    PIKA_TEST_EQ(int_f_count, 1u);
    PIKA_TEST_EQ(void_f1_count, 1u);
    PIKA_TEST_EQ(int_f1_count, 16u);
    PIKA_TEST_EQ(int_f2_count, 1u);
}

///////////////////////////////////////////////////////////////////////////////

std::atomic<std::uint32_t> future_void_f1_count;
std::atomic<std::uint32_t> future_void_f2_count;

void future_void_f1(future<void> f1)
{
    PIKA_TEST(f1.is_ready());
    ++future_void_f1_count;
}

void future_void_sf1(shared_future<void> f1)
{
    PIKA_TEST(f1.is_ready());
    ++future_void_f1_count;
}

void future_void_f2(future<void> f1, future<void> f2)
{
    PIKA_TEST(f1.is_ready());
    PIKA_TEST(f2.is_ready());
    ++future_void_f2_count;
}

std::atomic<std::uint32_t> future_int_f1_count;
std::atomic<std::uint32_t> future_int_f2_count;

int future_int_f1(future<void> f1)
{
    PIKA_TEST(f1.is_ready());
    ++future_int_f1_count;
    return 1;
}

int future_int_f2(future<int> f1, future<int> f2)
{
    PIKA_TEST(f1.is_ready());
    PIKA_TEST(f2.is_ready());
    ++future_int_f2_count;
    return f1.get() + f2.get();
}

std::atomic<std::uint32_t> future_int_f_vector_count;

int future_int_f_vector(std::vector<future<int>>& vf)
{
    int sum = 0;
    for (future<int>& f : vf)
    {
        PIKA_TEST(f.is_ready());
        sum += f.get();
    }
    ++future_int_f_vector_count;
    return sum;
}

template <typename Executor>
void future_function_pointers(Executor& exec)
{
    future_void_f1_count.store(0);
    future_void_f2_count.store(0);
    future_int_f1_count.store(0);
    future_int_f2_count.store(0);

    future<void> f1 = dataflow(
        exec, &future_void_f1, async(&future_void_sf1, shared_future<void>(make_ready_future())));

    f1.wait();

    PIKA_TEST_EQ(future_void_f1_count, 2u);
    future_void_f1_count.store(0);

    future<void> f2 = dataflow(exec, &future_void_f2,
        async(&future_void_sf1, shared_future<void>(make_ready_future())),
        async(&future_void_sf1, shared_future<void>(make_ready_future())));

    f2.wait();
    PIKA_TEST_EQ(future_void_f1_count, 2u);
    PIKA_TEST_EQ(future_void_f2_count, 1u);

    future_void_f1_count.store(0);
    future_void_f2_count.store(0);
    future_int_f1_count.store(0);
    future_int_f2_count.store(0);

    future<int> f3 = dataflow(exec, &future_int_f1, make_ready_future());

    PIKA_TEST_EQ(f3.get(), 1);
    PIKA_TEST_EQ(future_int_f1_count, 1u);
    future_int_f1_count.store(0);

    future<int> f4 =
        dataflow(exec, &future_int_f2, dataflow(exec, &future_int_f1, make_ready_future()),
            dataflow(exec, &future_int_f1, make_ready_future()));

    PIKA_TEST_EQ(f4.get(), 2);
    PIKA_TEST_EQ(future_int_f1_count, 2u);
    PIKA_TEST_EQ(future_int_f2_count, 1u);
    future_int_f1_count.store(0);
    future_int_f2_count.store(0);

    future_int_f_vector_count.store(0);
    std::vector<future<int>> vf;
    for (std::size_t i = 0; i < 10; ++i)
    {
        vf.push_back(dataflow(exec, &future_int_f1, make_ready_future()));
    }
    future<int> f5 = dataflow(exec, &future_int_f_vector, std::ref(vf));

    PIKA_TEST_EQ(f5.get(), 10);
    PIKA_TEST_EQ(future_int_f1_count, 10u);
    PIKA_TEST_EQ(future_int_f_vector_count, 1u);
}

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::uint32_t> void_f4_count;
std::atomic<std::uint32_t> int_f4_count;

void void_f4(int)
{
    ++void_f4_count;
}
int int_f4(int i)
{
    ++int_f4_count;
    return i + 42;
}

std::atomic<std::uint32_t> void_f5_count;
std::atomic<std::uint32_t> int_f5_count;

void void_f5(int, pika::future<int>)
{
    ++void_f5_count;
}
int int_f5(int i, pika::future<int> j)
{
    ++int_f5_count;
    return i + j.get() + 42;
}

template <typename Executor>
void plain_arguments(Executor& exec)
{
    void_f4_count.store(0);
    int_f4_count.store(0);

    {
        future<void> f1 = dataflow(exec, &void_f4, 42);
        future<int> f2 = dataflow(exec, &int_f4, 42);

        f1.wait();
        PIKA_TEST_EQ(void_f4_count, 1u);

        PIKA_TEST_EQ(f2.get(), 84);
        PIKA_TEST_EQ(int_f4_count, 1u);
    }

    void_f5_count.store(0);
    int_f5_count.store(0);

    {
        future<void> f1 = dataflow(exec, &void_f5, 42, async(&int_f));
        future<int> f2 = dataflow(exec, &int_f5, 42, async(&int_f));

        f1.wait();
        PIKA_TEST_EQ(void_f5_count, 1u);

        PIKA_TEST_EQ(f2.get(), 126);
        PIKA_TEST_EQ(int_f5_count, 1u);
    }
}

template <typename Executor>
void plain_deferred_arguments(Executor& exec)
{
    void_f5_count.store(0);
    int_f5_count.store(0);

    {
        future<void> f1 = dataflow(exec, &void_f5, 42, async(pika::launch::deferred, &int_f));
        future<int> f2 = dataflow(exec, &int_f5, 42, async(pika::launch::deferred, &int_f));

        f1.wait();
        PIKA_TEST_EQ(void_f5_count, 1u);

        PIKA_TEST_EQ(f2.get(), 126);
        PIKA_TEST_EQ(int_f5_count, 1u);
    }
}

///////////////////////////////////////////////////////////////////////////////
int pika_main(variables_map&)
{
    {
        pika::execution::sequenced_executor exec;
        function_pointers(exec);
        future_function_pointers(exec);
        plain_arguments(exec);
        plain_deferred_arguments(exec);
    }

    {
        pika::execution::parallel_executor exec;
        function_pointers(exec);
        future_function_pointers(exec);
        plain_arguments(exec);
        plain_deferred_arguments(exec);
    }

    return pika::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {"pika.os_threads=all"};

    // Initialize and run pika
    pika::init_params init_args;
    init_args.cfg = cfg;

    PIKA_TEST_EQ_MSG(
        pika::init(pika_main, argc, argv, init_args), 0, "pika main exited with non-zero status");
    return 0;
}
