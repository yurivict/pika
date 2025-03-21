//  Copyright (c) 2016 Lukas Troska
//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <chrono>
#include <cstddef>
#include <deque>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
struct make_unsigned_slowly
{
    unsigned id;

    unsigned operator()() const
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return id;
    }
};

template <class Container>
void test_wait_each_from_list()
{
    unsigned count = 10;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    Container futures1;
    Container futures2;

    for (unsigned j = 0; j < count; ++j)
    {
        futures1.push_back(pika::async(make_unsigned_slowly{j}));
        futures2.push_back(pika::async(make_unsigned_slowly{j}));
    }

    pika::wait_each(callback, futures1);
    pika::wait_each(callback_with_index, futures2);

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    for (const auto& f : futures1)
    {
        PIKA_TEST(!f.valid());
    }

    for (const auto& f : futures2)
    {
        PIKA_TEST(!f.valid());
    }
}

template <class Container>
void test_wait_each_from_list_iterators()
{
    unsigned count = 10;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    Container futures1;
    Container futures2;

    for (unsigned j = 0; j < count; ++j)
    {
        futures1.push_back(pika::async(make_unsigned_slowly{j}));
        futures2.push_back(pika::async(make_unsigned_slowly{j}));
    }

    pika::wait_each(callback, futures1.begin(), futures1.end());
    pika::wait_each(callback_with_index, futures2.begin(), futures2.end());

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    for (const auto& f : futures1)
    {
        PIKA_TEST(!f.valid());
    }

    for (const auto& f : futures2)
    {
        PIKA_TEST(!f.valid());
    }
}

template <class Container>
void test_wait_each_n_from_list_iterators()
{
    unsigned count = 10;
    unsigned n = 5;

    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback_n = [n, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, n);
    };

    auto callback_with_index_n = [n, &call_with_index_count](
                                     std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, n);
    };

    Container futures1;
    Container futures2;

    for (unsigned j = 0; j < count; ++j)
    {
        futures1.push_back(pika::async(make_unsigned_slowly{j}));
        futures2.push_back(pika::async(make_unsigned_slowly{j}));
    }

    pika::wait_each_n(callback_n, futures1.begin(), n);
    pika::wait_each_n(callback_with_index_n, futures2.begin(), n);

    PIKA_TEST_EQ(call_count, n);
    PIKA_TEST_EQ(call_with_index_count, n);

    unsigned num = 0;
    for (auto it = futures1.begin(); num < n; ++num, ++it)
    {
        PIKA_TEST(!it->valid());
    }

    num = 0;
    for (auto it = futures2.begin(); num < n; ++num, ++it)
    {
        PIKA_TEST(!it->valid());
    }
}

void test_wait_each_one_future()
{
    unsigned count = 1;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    pika::future<unsigned> f = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> g = pika::make_ready_future(static_cast<unsigned>(0));

    pika::wait_each(callback, std::move(f));
    pika::wait_each(callback_with_index, std::move(g));

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    PIKA_TEST(!f.valid());    // NOLINT
    PIKA_TEST(!g.valid());    // NOLINT
}

void test_wait_each_two_futures()
{
    unsigned count = 2;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    pika::future<unsigned> f1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> f2 = pika::make_ready_future(static_cast<unsigned>(1));
    pika::future<unsigned> g1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> g2 = pika::make_ready_future(static_cast<unsigned>(1));

    pika::wait_each(callback, std::move(f1), std::move(f2));
    pika::wait_each(callback_with_index, std::move(g1), std::move(g2));

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    PIKA_TEST(!f1.valid());    // NOLINT
    PIKA_TEST(!f2.valid());    // NOLINT
    PIKA_TEST(!g1.valid());    // NOLINT
    PIKA_TEST(!g2.valid());    // NOLINT
}

void test_wait_each_three_futures()
{
    unsigned count = 3;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    pika::future<unsigned> f1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> f2 = pika::make_ready_future(static_cast<unsigned>(1));
    pika::future<unsigned> f3 = pika::make_ready_future(static_cast<unsigned>(2));
    pika::future<unsigned> g1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> g2 = pika::make_ready_future(static_cast<unsigned>(1));
    pika::future<unsigned> g3 = pika::make_ready_future(static_cast<unsigned>(2));

    pika::wait_each(callback, std::move(f1), std::move(f2), std::move(f3));
    pika::wait_each(callback_with_index, std::move(g1), std::move(g2), std::move(g3));

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    PIKA_TEST(!f1.valid());    // NOLINT
    PIKA_TEST(!f2.valid());    // NOLINT
    PIKA_TEST(!f3.valid());    // NOLINT
    PIKA_TEST(!g1.valid());    // NOLINT
    PIKA_TEST(!g2.valid());    // NOLINT
    PIKA_TEST(!g3.valid());    // NOLINT
}

void test_wait_each_four_futures()
{
    unsigned count = 4;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    pika::future<unsigned> f1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> f2 = pika::make_ready_future(static_cast<unsigned>(1));
    pika::future<unsigned> f3 = pika::make_ready_future(static_cast<unsigned>(2));
    pika::future<unsigned> f4 = pika::make_ready_future(static_cast<unsigned>(3));
    pika::future<unsigned> g1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> g2 = pika::make_ready_future(static_cast<unsigned>(1));
    pika::future<unsigned> g3 = pika::make_ready_future(static_cast<unsigned>(2));
    pika::future<unsigned> g4 = pika::make_ready_future(static_cast<unsigned>(3));

    pika::wait_each(callback, std::move(f1), std::move(f2), std::move(f3), std::move(f4));
    pika::wait_each(
        callback_with_index, std::move(g1), std::move(g2), std::move(g3), std::move(g4));

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    PIKA_TEST(!f1.valid());    // NOLINT
    PIKA_TEST(!f2.valid());    // NOLINT
    PIKA_TEST(!f3.valid());    // NOLINT
    PIKA_TEST(!f4.valid());    // NOLINT
    PIKA_TEST(!g1.valid());    // NOLINT
    PIKA_TEST(!g2.valid());    // NOLINT
    PIKA_TEST(!g3.valid());    // NOLINT
    PIKA_TEST(!g4.valid());    // NOLINT
}

void test_wait_each_five_futures()
{
    unsigned count = 5;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    pika::future<unsigned> f1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> f2 = pika::make_ready_future(static_cast<unsigned>(1));
    pika::future<unsigned> f3 = pika::make_ready_future(static_cast<unsigned>(2));
    pika::future<unsigned> f4 = pika::make_ready_future(static_cast<unsigned>(3));
    pika::future<unsigned> f5 = pika::make_ready_future(static_cast<unsigned>(4));
    pika::future<unsigned> g1 = pika::make_ready_future(static_cast<unsigned>(0));
    pika::future<unsigned> g2 = pika::make_ready_future(static_cast<unsigned>(1));
    pika::future<unsigned> g3 = pika::make_ready_future(static_cast<unsigned>(2));
    pika::future<unsigned> g4 = pika::make_ready_future(static_cast<unsigned>(3));
    pika::future<unsigned> g5 = pika::make_ready_future(static_cast<unsigned>(4));

    pika::wait_each(
        callback, std::move(f1), std::move(f2), std::move(f3), std::move(f4), std::move(f5));

    pika::wait_each(callback_with_index, std::move(g1), std::move(g2), std::move(g3), std::move(g4),
        std::move(g5));

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    PIKA_TEST(!f1.valid());    // NOLINT
    PIKA_TEST(!f2.valid());    // NOLINT
    PIKA_TEST(!f3.valid());    // NOLINT
    PIKA_TEST(!f4.valid());    // NOLINT
    PIKA_TEST(!f5.valid());    // NOLINT
    PIKA_TEST(!g1.valid());    // NOLINT
    PIKA_TEST(!g2.valid());    // NOLINT
    PIKA_TEST(!g3.valid());    // NOLINT
    PIKA_TEST(!g4.valid());    // NOLINT
    PIKA_TEST(!g5.valid());    // NOLINT
}

void test_wait_each_late_future()
{
    unsigned count = 2;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    pika::lcos::local::futures_factory<unsigned()> pt0(make_unsigned_slowly{0});
    pika::lcos::local::futures_factory<unsigned()> pt1(make_unsigned_slowly{1});
    pika::lcos::local::futures_factory<unsigned()> pt2(make_unsigned_slowly{0});
    pika::lcos::local::futures_factory<unsigned()> pt3(make_unsigned_slowly{1});

    pika::future<unsigned> f1 = pt0.get_future();
    pika::future<unsigned> f2 = pt1.get_future();

    pika::async([pt0 = std::move(pt0)]() { pt0.apply(); });
    pika::async([pt1 = std::move(pt1)]() { pt1.apply(); });

    pika::wait_each(callback, std::move(f1), std::move(f2));

    PIKA_TEST(!f1.valid());    // NOLINT
    PIKA_TEST(!f2.valid());    // NOLINT

    pika::future<unsigned> g1 = pt2.get_future();
    pika::future<unsigned> g2 = pt3.get_future();

    pika::async([pt2 = std::move(pt2)]() { pt2.apply(); });
    pika::async([pt3 = std::move(pt3)]() { pt3.apply(); });

    pika::wait_each(callback_with_index, std::move(g1), std::move(g2));

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    PIKA_TEST(!g1.valid());    // NOLINT
    PIKA_TEST(!g2.valid());    // NOLINT
}

void test_wait_each_deferred_futures()
{
    unsigned count = 2;
    unsigned call_count = 0;
    unsigned call_with_index_count = 0;

    auto callback = [count, &call_count](pika::future<unsigned> fut) {
        ++call_count;

        unsigned id = fut.get();

        PIKA_TEST_LT(id, count);
    };

    auto callback_with_index = [count, &call_with_index_count](
                                   std::size_t idx, pika::future<unsigned> fut) {
        ++call_with_index_count;

        unsigned id = fut.get();

        PIKA_TEST_EQ(idx, id);
        PIKA_TEST_LT(id, count);
    };

    pika::future<unsigned> f1 = pika::async(pika::launch::deferred, make_unsigned_slowly{0});
    pika::future<unsigned> f2 = pika::async(pika::launch::deferred, make_unsigned_slowly{1});

    pika::future<unsigned> g1 = pika::async(pika::launch::deferred, make_unsigned_slowly{0});
    pika::future<unsigned> g2 = pika::async(pika::launch::deferred, make_unsigned_slowly{1});

    pika::wait_each(callback, std::move(f1), std::move(f2));
    pika::wait_each(callback_with_index, std::move(g1), std::move(g2));

    PIKA_TEST_EQ(call_count, count);
    PIKA_TEST_EQ(call_with_index_count, count);

    PIKA_TEST(!f1.valid());    // NOLINT
    PIKA_TEST(!f2.valid());    // NOLINT

    PIKA_TEST(!g1.valid());    // NOLINT
    PIKA_TEST(!g2.valid());    // NOLINT
}

///////////////////////////////////////////////////////////////////////////////
using pika::program_options::options_description;
using pika::program_options::variables_map;

using pika::future;

int pika_main(variables_map&)
{
    {
        test_wait_each_from_list<std::vector<future<unsigned>>>();

        test_wait_each_from_list_iterators<std::vector<future<unsigned>>>();
        test_wait_each_from_list_iterators<std::list<future<unsigned>>>();
        test_wait_each_from_list_iterators<std::deque<future<unsigned>>>();

        test_wait_each_n_from_list_iterators<std::vector<future<unsigned>>>();
        test_wait_each_n_from_list_iterators<std::list<future<unsigned>>>();
        test_wait_each_n_from_list_iterators<std::deque<future<unsigned>>>();

        test_wait_each_one_future();
        test_wait_each_two_futures();
        test_wait_each_three_futures();
        test_wait_each_four_futures();
        test_wait_each_five_futures();

        test_wait_each_late_future();

        test_wait_each_deferred_futures();
    }

    pika::finalize();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    options_description cmdline("Usage: " PIKA_APPLICATION_STRING " [options]");

    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {"pika.os_threads=all"};

    // Initialize and run pika
    pika::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return pika::init(pika_main, argc, argv, init_args);
}
