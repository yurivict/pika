// (C) Copyright 2006-7 Anthony Williams
//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <pika/async/apply.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/shared_mutex.hpp>
#include <pika/thread.hpp>

#include <pika/testing.hpp>

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "shared_mutex_locking_thread.hpp"
#include "thread_group.hpp"

#define CHECK_LOCKED_VALUE_EQUAL(mutex_name, value, expected_value)                                \
 {                                                                                                 \
  std::unique_lock<pika::mutex> lock(mutex_name);                                                  \
  PIKA_TEST_EQ(value, expected_value);                                                             \
 }

void test_multiple_readers()
{
    using shared_mutex_type = pika::shared_mutex;
    using mutex_type = pika::mutex;

    unsigned const number_of_threads = 10;

    test::thread_group pool;

    pika::shared_mutex rw_mutex;
    unsigned unblocked_count = 0;
    unsigned simultaneous_running_count = 0;
    unsigned max_simultaneous_running = 0;
    mutex_type unblocked_count_mutex;
    pika::condition_variable unblocked_condition;
    mutex_type finish_mutex;
    std::unique_lock<mutex_type> finish_lock(finish_mutex);

    try
    {
        for (unsigned i = 0; i != number_of_threads; ++i)
        {
            pool.create_thread(test::locking_thread<std::shared_lock<shared_mutex_type>>(rw_mutex,
                unblocked_count, unblocked_count_mutex, unblocked_condition, finish_mutex,
                simultaneous_running_count, max_simultaneous_running));
        }

        {
            std::unique_lock<mutex_type> lk(unblocked_count_mutex);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (unblocked_count < number_of_threads)
            {
                unblocked_condition.wait(lk);
            }
        }

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, number_of_threads);

        finish_lock.unlock();
        pool.join_all();
    }
    catch (...)
    {
        pool.interrupt_all();
        pool.join_all();
        PIKA_TEST(false);
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, max_simultaneous_running, number_of_threads);
}

void test_only_one_writer_permitted()
{
    using shared_mutex_type = pika::shared_mutex;
    using mutex_type = pika::mutex;

    unsigned const number_of_threads = 10;

    test::thread_group pool;

    pika::shared_mutex rw_mutex;
    unsigned unblocked_count = 0;
    unsigned simultaneous_running_count = 0;
    unsigned max_simultaneous_running = 0;
    mutex_type unblocked_count_mutex;
    pika::condition_variable unblocked_condition;
    mutex_type finish_mutex;
    std::unique_lock<mutex_type> finish_lock(finish_mutex);

    try
    {
        for (unsigned i = 0; i != number_of_threads; ++i)
        {
            pool.create_thread(test::locking_thread<std::unique_lock<shared_mutex_type>>(rw_mutex,
                unblocked_count, unblocked_count_mutex, unblocked_condition, finish_mutex,
                simultaneous_running_count, max_simultaneous_running));
        }

        pika::this_thread::yield();

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, 1u);

        finish_lock.unlock();
        pool.join_all();
    }
    catch (...)
    {
        pool.interrupt_all();
        pool.join_all();
        PIKA_TEST(false);
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, number_of_threads);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, max_simultaneous_running, 1u);
}

void test_reader_blocks_writer()
{
    using shared_mutex_type = pika::shared_mutex;
    using mutex_type = pika::mutex;

    test::thread_group pool;

    pika::shared_mutex rw_mutex;
    unsigned unblocked_count = 0;
    unsigned simultaneous_running_count = 0;
    unsigned max_simultaneous_running = 0;
    mutex_type unblocked_count_mutex;
    pika::condition_variable unblocked_condition;
    mutex_type finish_mutex;
    std::unique_lock<mutex_type> finish_lock(finish_mutex);

    try
    {
        pool.create_thread(test::locking_thread<std::shared_lock<shared_mutex_type>>(rw_mutex,
            unblocked_count, unblocked_count_mutex, unblocked_condition, finish_mutex,
            simultaneous_running_count, max_simultaneous_running));

        {
            std::unique_lock<mutex_type> lk(unblocked_count_mutex);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (unblocked_count < 1)
            {
                unblocked_condition.wait(lk);
            }
        }

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, 1u);

        pool.create_thread(test::locking_thread<std::unique_lock<shared_mutex_type>>(rw_mutex,
            unblocked_count, unblocked_count_mutex, unblocked_condition, finish_mutex,
            simultaneous_running_count, max_simultaneous_running));

        pika::this_thread::yield();

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, 1u);

        finish_lock.unlock();
        pool.join_all();
    }
    catch (...)
    {
        pool.interrupt_all();
        pool.join_all();
        PIKA_TEST(false);
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, 2u);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, max_simultaneous_running, 1u);
}

void test_unlocking_writer_unblocks_all_readers()
{
    using shared_mutex_type = pika::shared_mutex;
    using mutex_type = pika::mutex;

    test::thread_group pool;

    pika::shared_mutex rw_mutex;
    std::unique_lock<pika::shared_mutex> write_lock(rw_mutex);
    unsigned unblocked_count = 0;
    unsigned simultaneous_running_count = 0;
    unsigned max_simultaneous_running = 0;
    mutex_type unblocked_count_mutex;
    pika::condition_variable unblocked_condition;
    mutex_type finish_mutex;
    std::unique_lock<mutex_type> finish_lock(finish_mutex);

    unsigned const reader_count = 10;

    try
    {
        for (unsigned i = 0; i != reader_count; ++i)
        {
            pool.create_thread(test::locking_thread<std::shared_lock<shared_mutex_type>>(rw_mutex,
                unblocked_count, unblocked_count_mutex, unblocked_condition, finish_mutex,
                simultaneous_running_count, max_simultaneous_running));
        }

        pika::this_thread::yield();

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, 0u);

        write_lock.unlock();

        {
            std::unique_lock<mutex_type> lk(unblocked_count_mutex);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (unblocked_count < reader_count)
            {
                unblocked_condition.wait(lk);
            }
        }

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, reader_count);

        finish_lock.unlock();
        pool.join_all();
    }
    catch (...)
    {
        pool.interrupt_all();
        pool.join_all();
        PIKA_TEST(false);
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, max_simultaneous_running, reader_count);
}

void test_unlocking_last_reader_only_unblocks_one_writer()
{
    using shared_mutex_type = pika::shared_mutex;
    using mutex_type = pika::mutex;

    test::thread_group pool;

    pika::shared_mutex rw_mutex;
    unsigned unblocked_count = 0;
    unsigned simultaneous_running_readers = 0;
    unsigned max_simultaneous_readers = 0;
    unsigned simultaneous_running_writers = 0;
    unsigned max_simultaneous_writers = 0;
    mutex_type unblocked_count_mutex;
    pika::condition_variable unblocked_condition;
    mutex_type finish_reading_mutex;
    std::unique_lock<mutex_type> finish_reading_lock(finish_reading_mutex);
    mutex_type finish_writing_mutex;
    std::unique_lock<mutex_type> finish_writing_lock(finish_writing_mutex);

    unsigned const reader_count = 10;
    unsigned const writer_count = 10;

    try
    {
        for (unsigned i = 0; i != reader_count; ++i)
        {
            pool.create_thread(test::locking_thread<std::shared_lock<shared_mutex_type>>(rw_mutex,
                unblocked_count, unblocked_count_mutex, unblocked_condition, finish_reading_mutex,
                simultaneous_running_readers, max_simultaneous_readers));
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        pika::this_thread::yield();

        for (unsigned i = 0; i != writer_count; ++i)
        {
            pool.create_thread(test::locking_thread<std::unique_lock<shared_mutex_type>>(rw_mutex,
                unblocked_count, unblocked_count_mutex, unblocked_condition, finish_writing_mutex,
                simultaneous_running_writers, max_simultaneous_writers));
        }

        {
            std::unique_lock<mutex_type> lk(unblocked_count_mutex);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (unblocked_count < reader_count)
            {
                unblocked_condition.wait(lk);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
        pika::this_thread::yield();

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, reader_count);

        finish_reading_lock.unlock();

        {
            std::unique_lock<mutex_type> lk(unblocked_count_mutex);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (unblocked_count < (reader_count + 1))
            {
                unblocked_condition.wait(lk);
            }
        }

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, reader_count + 1);

        finish_writing_lock.unlock();
        pool.join_all();
    }
    catch (...)
    {
        pool.interrupt_all();
        pool.join_all();
        PIKA_TEST(false);
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, unblocked_count, reader_count + writer_count);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, max_simultaneous_readers, reader_count);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex, max_simultaneous_writers, 1u);
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    test_multiple_readers();
    test_only_one_writer_permitted();
    test_reader_blocks_writer();
    test_unlocking_writer_unblocks_all_readers();
    test_unlocking_last_reader_only_unblocks_one_writer();

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"pika.os_threads=all"};

    // Initialize and run pika
    pika::init_params init_args;
    init_args.cfg = cfg;
    PIKA_TEST_EQ_MSG(
        pika::init(pika_main, argc, argv, init_args), 0, "pika main exited with non-zero status");

    return 0;
}
