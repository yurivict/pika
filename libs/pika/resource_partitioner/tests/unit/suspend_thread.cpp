//  Copyright (c) 2017 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Simple test verifying basic resource_partitioner functionality.

#include <pika/assert.hpp>
#include <pika/chrono.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/modules/resource_partitioner.hpp>
#include <pika/modules/schedulers.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>
#include <pika/thread_pool_util/thread_pool_suspension_helpers.hpp>
#include <pika/threading_base/scheduler_mode.hpp>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

std::size_t const max_threads =
    (std::min)(std::size_t(4), std::size_t(pika::threads::detail::hardware_concurrency()));

int pika_main()
{
    std::size_t const num_threads = pika::resource::get_num_threads("default");

    PIKA_TEST_EQ(std::size_t(max_threads), num_threads);

    pika::threads::detail::thread_pool_base& tp = pika::resource::get_thread_pool("default");

    PIKA_TEST_EQ(tp.get_active_os_thread_count(), std::size_t(max_threads));

    {
        // Check number of used resources
        for (std::size_t thread_num = 0; thread_num < num_threads - 1; ++thread_num)
        {
            pika::threads::detail::suspend_processing_unit(tp, thread_num).get();
            PIKA_TEST_EQ(
                std::size_t(num_threads - thread_num - 1), tp.get_active_os_thread_count());
        }

        for (std::size_t thread_num = 0; thread_num < num_threads - 1; ++thread_num)
        {
            pika::threads::detail::resume_processing_unit(tp, thread_num).get();
            PIKA_TEST_EQ(std::size_t(thread_num + 2), tp.get_active_os_thread_count());
        }
    }

    {
        // Check suspending pu on which current thread is running.

        // NOTE: This only works as long as there is another OS thread which has
        // no work and is able to steal.
        std::size_t worker_thread_num = pika::get_worker_thread_num();
        pika::threads::detail::suspend_processing_unit(tp, worker_thread_num).get();
        pika::threads::detail::resume_processing_unit(tp, worker_thread_num).get();
    }

    {
        // Check when suspending all but one, we end up on the same thread
        std::size_t thread_num = 0;
        auto test_function = [&thread_num, &tp]() {
            PIKA_TEST_EQ(thread_num + tp.get_thread_offset(), pika::get_worker_thread_num());
        };

        for (thread_num = 0; thread_num < num_threads; ++thread_num)
        {
            for (std::size_t thread_num_suspend = 0; thread_num_suspend < num_threads;
                 ++thread_num_suspend)
            {
                if (thread_num != thread_num_suspend)
                {
                    pika::threads::detail::suspend_processing_unit(tp, thread_num_suspend).get();
                }
            }

            pika::async(test_function).wait();

            for (std::size_t thread_num_resume = 0; thread_num_resume < num_threads;
                 ++thread_num_resume)
            {
                if (thread_num != thread_num_resume)
                {
                    pika::threads::detail::resume_processing_unit(tp, thread_num_resume).get();
                }
            }
        }
    }

    {
        // Check suspending and resuming the same thread without waiting for
        // each to finish.
        for (std::size_t thread_num = 0; thread_num < pika::resource::get_num_threads("default");
             ++thread_num)
        {
            std::vector<pika::future<void>> fs;

            fs.push_back(pika::threads::detail::suspend_processing_unit(tp, thread_num));
            fs.push_back(pika::threads::detail::resume_processing_unit(tp, thread_num));

            pika::wait_all(fs);

            // Suspend is not guaranteed to run before resume, so make sure
            // processing unit is running
            pika::threads::detail::resume_processing_unit(tp, thread_num).get();

            fs.clear();

            // Launching the same number of tasks as worker threads may deadlock
            // as no thread is available to steal from the current thread.
            for (std::size_t i = 0; i < max_threads - 1; ++i)
            {
                fs.push_back(pika::threads::detail::suspend_processing_unit(tp, thread_num));
            }

            pika::wait_all(fs);

            fs.clear();

            // Launching the same number of tasks as worker threads may deadlock
            // as no thread is available to steal from the current thread.
            for (std::size_t i = 0; i < max_threads - 1; ++i)
            {
                fs.push_back(pika::threads::detail::resume_processing_unit(tp, thread_num));
            }

            pika::wait_all(fs);
        }
    }

    {
        // Check random scheduling with reducing resources.
        std::size_t thread_num = 0;
        bool up = true;
        std::vector<pika::future<void>> fs;
        pika::chrono::detail::high_resolution_timer t;
        while (t.elapsed() < 2)
        {
            for (std::size_t i = 0; i < pika::resource::get_num_threads("default") * 10; ++i)
            {
                fs.push_back(pika::async([]() {}));
            }

            if (up)
            {
                if (thread_num < pika::resource::get_num_threads("default") - 1)
                {
                    pika::threads::detail::suspend_processing_unit(tp, thread_num).get();
                }

                ++thread_num;

                if (thread_num == pika::resource::get_num_threads("default"))
                {
                    up = false;
                    --thread_num;
                }
            }
            else
            {
                pika::threads::detail::resume_processing_unit(tp, thread_num).get();

                if (thread_num > 0)
                {
                    --thread_num;
                }
                else
                {
                    up = true;
                }
            }
        }

        pika::when_all(std::move(fs)).get();

        // Don't exit with suspended pus
        for (std::size_t thread_num_resume = 0; thread_num_resume < thread_num; ++thread_num_resume)
        {
            pika::threads::detail::resume_processing_unit(tp, thread_num_resume).get();
        }
    }

    return pika::finalize();
}

void test_scheduler(int argc, char* argv[], pika::resource::scheduling_policy scheduler)
{
    using ::pika::threads::scheduler_mode;
    pika::init_params init_args;
    init_args.cfg = {"pika.os_threads=" + std::to_string(max_threads)};
    init_args.rp_callback = [scheduler](auto& rp, pika::program_options::variables_map const&) {
        rp.create_thread_pool(
            "default", scheduler, scheduler_mode::default_mode | scheduler_mode::enable_elasticity);
    };

    PIKA_TEST_EQ(pika::init(pika_main, argc, argv, init_args), 0);
}

int main(int argc, char* argv[])
{
    PIKA_ASSERT(max_threads >= 2);

    // NOTE: Static schedulers do not support suspending the own worker thread
    // because they do not steal work.

    {
        // These schedulers should succeed
        std::vector<pika::resource::scheduling_policy> schedulers = {
            pika::resource::scheduling_policy::local,
            pika::resource::scheduling_policy::local_priority_fifo,
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
            pika::resource::scheduling_policy::local_priority_lifo,
#endif
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
            pika::resource::scheduling_policy::abp_priority_fifo,
            pika::resource::scheduling_policy::abp_priority_lifo,
#endif
            pika::resource::scheduling_policy::shared_priority,
        };

        for (auto const scheduler : schedulers)
        {
            test_scheduler(argc, argv, scheduler);
        }
    }

    {
        // These schedulers should fail
        std::vector<pika::resource::scheduling_policy> schedulers = {
            pika::resource::scheduling_policy::static_,
            pika::resource::scheduling_policy::static_priority,
        };

        for (auto const scheduler : schedulers)
        {
            bool exception_thrown = false;
            try
            {
                test_scheduler(argc, argv, scheduler);
            }
            catch (pika::exception const&)
            {
                exception_thrown = true;
            }

            PIKA_TEST(exception_thrown);
        }
    }

    return 0;
}
