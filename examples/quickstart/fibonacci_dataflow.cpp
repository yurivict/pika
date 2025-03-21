//  Copyright (c)      2013 Thomas Heller
//  Copyright (c) 2007-2013 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is a purely local version demonstrating the proposed extension to
// C++ implementing resumable functions (see N3564,
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3564.pdf). The
// necessary transformations are performed by hand.

#include <pika/chrono.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>

#include <fmt/ostream.h>
#include <fmt/printf.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
std::uint64_t threshold = 2;

///////////////////////////////////////////////////////////////////////////////
PIKA_NOINLINE std::uint64_t fibonacci_serial(std::uint64_t n)
{
    if (n < 2)
        return n;
    return fibonacci_serial(n - 1) + fibonacci_serial(n - 2);
}

///////////////////////////////////////////////////////////////////////////////
pika::future<std::uint64_t> fibonacci(std::uint64_t n)
{
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    pika::future<std::uint64_t> lhs_future = pika::async(&fibonacci, n - 1);
    pika::future<std::uint64_t> rhs_future = fibonacci(n - 2);

    return pika::dataflow(
        pika::unwrapping([](std::uint64_t lhs, std::uint64_t rhs) { return lhs + rhs; }),
        std::move(lhs_future), std::move(rhs_future));
}

///////////////////////////////////////////////////////////////////////////////
int pika_main(pika::program_options::variables_map& vm)
{
    pika::scoped_finalize f;

    // extract command line argument, i.e. fib(N)
    std::uint64_t n = vm["n-value"].as<std::uint64_t>();
    std::string test = vm["test"].as<std::string>();
    std::uint64_t max_runs = vm["n-runs"].as<std::uint64_t>();

    if (max_runs == 0)
    {
        std::cerr << "fibonacci_dataflow: wrong command line argument value for option 'n-runs', "
                     "should not be zero"
                  << std::endl;
        return -1;
    }

    threshold = vm["threshold"].as<unsigned int>();
    if (threshold < 2 || threshold > n)
    {
        std::cerr << "fibonacci_dataflow: wrong command line argument value for option "
                     "'threshold', should be in between 2 and n-value, value specified: "
                  << threshold << std::endl;
        return -1;
    }

    bool executed_one = false;
    std::uint64_t r = 0;

    using namespace std::chrono;
    if (test == "all" || test == "0")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::uint64_t i = 0; i != max_runs; ++i)
        {
            // serial execution
            r = fibonacci_serial(n);
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_serial({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "1")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::uint64_t i = 0; i != max_runs; ++i)
        {
            // Create a future for the whole calculation, execute it locally,
            // and wait for it.
            r = fibonacci(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_await({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (!executed_one)
    {
        std::cerr << "fibonacci_dataflow: wrong command line argument value for option 'tests', "
                     "should be either 'all' or a number between zero and 1, value specified: "
                  << test << std::endl;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    pika::program_options::options_description desc_commandline(
        "Usage: " PIKA_APPLICATION_STRING " [options]");

    using pika::program_options::value;
    // clang-format off
    desc_commandline.add_options()
        ("n-value", value<std::uint64_t>()->default_value(10),
         "n value for the Fibonacci function")
        ("n-runs", value<std::uint64_t>()->default_value(1),
         "number of runs to perform")
        ("threshold", value<unsigned int>()->default_value(2),
         "threshold for switching to serial code")
        ("test", value<std::string>()->default_value("all"),
        "select tests to execute (0-1, default: all)");
    // clang-format on

    // Initialize and run pika
    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return pika::init(pika_main, argc, argv, init_args);
}
