//  Copyright (c) 2018 Mikael Simberg
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example benchmarks the time it takes to start and stop the pika runtime.
// This is meant to be compared to resume_suspend and openmp_parallel_region.

#include <pika/chrono.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>
#include <pika/program_options.hpp>
#include <pika/testing/performance.hpp>
#include <pika/thread.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>

int pika_main()
{
    return pika::finalize();
}

int main(int argc, char** argv)
{
    pika::program_options::options_description desc_commandline;
    desc_commandline.add_options()("repetitions",
        pika::program_options::value<std::uint64_t>()->default_value(100), "Number of repetitions");

    pika::program_options::variables_map vm;
    pika::program_options::store(pika::program_options::command_line_parser(argc, argv)
                                     .allow_unregistered()
                                     .options(desc_commandline)
                                     .run(),
        vm);

    std::uint64_t repetitions = vm["repetitions"].as<std::uint64_t>();

    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    pika::start(pika_main, argc, argv, init_args);
    std::uint64_t threads = pika::resource::get_num_threads("default");
    pika::stop();

    std::cout << "threads, resume [s], apply [s], suspend [s]" << std::endl;

    double start_time = 0;
    double stop_time = 0;
    pika::chrono::detail::high_resolution_timer timer;

    for (std::size_t i = 0; i < repetitions; ++i)
    {
        timer.restart();

        pika::init_params init_args;
        init_args.desc_cmdline = desc_commandline;

        pika::start(pika_main, argc, argv, init_args);
        auto t_start = timer.elapsed();
        start_time += t_start;

        for (std::size_t thread = 0; thread < threads; ++thread)
        {
            pika::apply([]() {});
        }

        auto t_apply = timer.elapsed();

        pika::stop();
        auto t_stop = timer.elapsed();
        stop_time += t_stop;

        std::cout << threads << ", " << t_start << ", " << t_apply << ", " << t_stop << std::endl;
    }
    pika::util::print_cdash_timing("StartTime", start_time);
    pika::util::print_cdash_timing("StopTime", stop_time);
}
