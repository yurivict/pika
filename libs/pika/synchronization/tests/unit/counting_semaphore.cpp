//  Copyright (c) 2020 Hartmut Kaiser
//  Copyright (C) 2013 Tim Blechmann
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/init.hpp>
#include <pika/semaphore.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <chrono>
#include <thread>

void test_semaphore_release_acquire()
{
    pika::counting_semaphore<> sem(1);

    sem.release();
    sem.acquire();
}

void test_semaphore_try_acquire()
{
    pika::counting_semaphore<> sem(0);

    PIKA_TEST(!sem.try_acquire());
    sem.release();
    PIKA_TEST(sem.try_acquire());
}

void test_semaphore_initial_count()
{
    pika::counting_semaphore<> sem(2);

    PIKA_TEST(sem.try_acquire());
    PIKA_TEST(sem.try_acquire());
    PIKA_TEST(!sem.try_acquire());
}

struct semaphore_acquire_and_release_test
{
    semaphore_acquire_and_release_test()
      : sem_(1)
    {
    }

    void run()
    {
        pika::thread release_thread(&semaphore_acquire_and_release_test::acquire_and_release, this);
        sem_.acquire();
        release_thread.join();
    }

    void acquire_and_release()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sem_.release();
    }

    pika::counting_semaphore<> sem_;
};

void test_semaphore_acquire_and_release()
{
    semaphore_acquire_and_release_test test;
    test.run();
}

void test_semaphore_try_acquire_for()
{
    pika::counting_semaphore<> sem(0);

    auto start = std::chrono::system_clock::now();

    PIKA_TEST(!sem.try_acquire_for(std::chrono::milliseconds(500)));

    auto end = std::chrono::system_clock::now();
    auto acquire_time = end - start;

    // guessing!
    PIKA_TEST(acquire_time > std::chrono::milliseconds(450));
    PIKA_TEST(acquire_time < std::chrono::milliseconds(1000));

    sem.release();

    PIKA_TEST(sem.try_acquire_for(std::chrono::milliseconds(500)));
}

void test_semaphore_try_acquire_until()
{
    pika::counting_semaphore<> sem(0);

    {
        auto now = std::chrono::system_clock::now();
        auto timeout = now + std::chrono::milliseconds(500);

        PIKA_TEST(!sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();
        auto timeout_delta = end - timeout;

        // guessing!
        PIKA_TEST(timeout_delta > std::chrono::milliseconds(-400));
        PIKA_TEST(timeout_delta < std::chrono::milliseconds(400));
    }

    sem.release();

    {
        auto start = std::chrono::system_clock::now();
        auto timeout = start + std::chrono::milliseconds(500);

        PIKA_TEST(sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();

        // guessing!
        PIKA_TEST((end - start) < std::chrono::milliseconds(100));
    }
}

void test_semaphore_try_acquire_for_until()
{
    pika::counting_semaphore<> sem(0);

    // Relative timeouts
    {
        auto start = std::chrono::system_clock::now();

        PIKA_TEST(!sem.try_acquire_for(std::chrono::milliseconds(500)));

        auto end = std::chrono::system_clock::now();
        auto acquire_time = end - start;

        // guessing!
        PIKA_TEST(acquire_time > std::chrono::milliseconds(450));
        PIKA_TEST(acquire_time < std::chrono::milliseconds(1000));

        sem.release();

        PIKA_TEST(sem.try_acquire_for(std::chrono::milliseconds(500)));
    }

    // Absolute timeouts
    {
        auto now = std::chrono::system_clock::now();
        auto timeout = now + std::chrono::milliseconds(500);

        PIKA_TEST(!sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();
        auto timeout_delta = end - timeout;

        // guessing!
        PIKA_TEST(timeout_delta > std::chrono::milliseconds(-400));
        PIKA_TEST(timeout_delta < std::chrono::milliseconds(400));
    }

    sem.release();

    {
        auto start = std::chrono::system_clock::now();
        auto timeout = start + std::chrono::milliseconds(500);

        PIKA_TEST(sem.try_acquire_until(timeout));

        auto end = std::chrono::system_clock::now();

        // guessing!
        PIKA_TEST((end - start) < std::chrono::milliseconds(100));
    }

    sem.release();

    {
        // timed acquire after timeout
        pika::counting_semaphore<> sema(1);

        auto start = std::chrono::steady_clock::now();
        auto timeout = start + std::chrono::milliseconds(100);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        sema.release();

        PIKA_TEST(sema.try_acquire_until(timeout));
    }
}

int pika_main()
{
    test_semaphore_release_acquire();
    test_semaphore_try_acquire();
    test_semaphore_initial_count();
    test_semaphore_acquire_and_release();
    test_semaphore_try_acquire_for();
    test_semaphore_try_acquire_until();
    test_semaphore_try_acquire_for_until();

    pika::finalize();
    return 0;
}

int main(int argc, char* argv[])
{
    return pika::init(pika_main, argc, argv);
}
