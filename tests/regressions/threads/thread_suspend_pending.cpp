//  Copyright (c) 2007-2011 Hartmut Kaiser
//  Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/barrier.hpp>
#include <pika/functional.hpp>
#include <pika/init.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

using pika::program_options::options_description;
using pika::program_options::value;
using pika::program_options::variables_map;

using pika::threads::detail::register_work;

using pika::barrier;

///////////////////////////////////////////////////////////////////////////////
void suspend_test(barrier<>& b, std::size_t iterations)
{
    for (std::size_t i = 0; i < iterations; ++i)
    {
        // Enter the 'pending' state and get rescheduled.
        pika::this_thread::suspend(
            pika::threads::detail::thread_schedule_state::pending, "suspend_test");
    }

    // Wait for all pika threads to enter the barrier.
    b.arrive_and_drop();
}

///////////////////////////////////////////////////////////////////////////////
int pika_main(variables_map& vm)
{
    std::size_t pxthreads = 0;

    if (vm.count("pxthreads"))
        pxthreads = vm["pxthreads"].as<std::size_t>();

    std::size_t iterations = 0;

    if (vm.count("iterations"))
        iterations = vm["iterations"].as<std::size_t>();

    {
        barrier<> b(pxthreads + 1);

        // Create the pika threads.
        for (std::size_t i = 0; i < pxthreads; ++i)
        {
            pika::threads::detail::thread_init_data data(
                pika::threads::detail::make_thread_function_nullary(
                    pika::util::detail::bind(&suspend_test, std::ref(b), iterations)),
                "suspend_test");
            register_work(data);
        }

        b.arrive_and_wait();    // Wait for all pika threads to enter the barrier.
    }

    // Initiate shutdown of the runtime system.
    return pika::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    options_description desc_commandline("Usage: " PIKA_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("pxthreads,T", value<std::size_t>()->default_value(0x100),
        "the number of PX threads to invoke")("iterations", value<std::size_t>()->default_value(64),
        "the number of iterations to execute in each thread");

    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {"pika.os_threads=all"};

    // Initialize and run pika
    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    PIKA_TEST_EQ_MSG(
        pika::init(pika_main, argc, argv, init_args), 0, "pika main exited with non-zero status");
    return 0;
}
