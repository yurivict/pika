//  Copyright (c) 2017-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

int make_int_slowly()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 42;
}

pika::future<int> make_future()
{
    pika::lcos::local::packaged_task<int()> task(make_int_slowly);
    return task.get_future();
}

void test_wait_any()
{
    {
        std::vector<pika::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(make_future());

        pika::wait_any_nothrow(future_array);

        int count = 0;
        for (auto& f : future_array)
        {
            if (f.is_ready())
            {
                ++count;
            }
        }
        PIKA_TEST_NEQ(count, 0);
    }
    {
        auto f1 = make_future();
        auto f2 = make_future();

        pika::wait_any_nothrow(f1, f2);

        PIKA_TEST(f1.is_ready() || f2.is_ready());
    }
    {
        std::vector<pika::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(pika::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            pika::wait_any_nothrow(future_array);

            int count = 0;
            for (auto& f : future_array)
            {
                if (f.is_ready())
                {
                    ++count;
                }
            }
            PIKA_TEST_NEQ(count, 0);
        }
        catch (std::runtime_error const&)
        {
            PIKA_TEST(false);
            caught_exception = true;
        }
        catch (...)
        {
            PIKA_TEST(false);
        }
        PIKA_TEST(!caught_exception);
    }
    {
        std::vector<pika::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(pika::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            pika::wait_any(future_array);
            PIKA_TEST(false);
        }
        catch (std::runtime_error const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            PIKA_TEST(false);
        }
        PIKA_TEST(caught_exception);
    }
}

void test_wait_any_n()
{
    {
        std::vector<pika::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(make_future());

        pika::wait_any_n_nothrow(future_array.begin(), future_array.size());

        int count = 0;
        for (auto& f : future_array)
        {
            if (f.is_ready())
            {
                ++count;
            }
        }
        PIKA_TEST_NEQ(count, 0);
    }
    {
        std::vector<pika::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(pika::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            pika::wait_any_n_nothrow(future_array.begin(), future_array.size());

            int count = 0;
            for (auto& f : future_array)
            {
                if (f.is_ready())
                {
                    ++count;
                }
            }
            PIKA_TEST_NEQ(count, 0);
        }
        catch (std::runtime_error const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            PIKA_TEST(false);
        }
        PIKA_TEST(!caught_exception);
    }
    {
        std::vector<pika::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(pika::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            pika::wait_any_n(future_array.begin(), future_array.size());
            PIKA_TEST(false);
        }
        catch (std::runtime_error const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            PIKA_TEST(false);
        }
        PIKA_TEST(caught_exception);
    }
}

int pika_main()
{
    test_wait_any();
    test_wait_any_n();
    return pika::finalize();
}

int main(int argc, char* argv[])
{
    PIKA_TEST_EQ(pika::init(pika_main, argc, argv), 0);
    return 0;
}
