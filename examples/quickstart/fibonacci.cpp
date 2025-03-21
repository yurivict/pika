////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

// This is example is equivalent to fibonacci.cpp, except that this example does
// not use actions (only plain functions). Many more variations are found in
// fibonacci_futures.cpp. This example is mainly intended to demonstrate async,
// futures and get for the documentation.

#include <pika/chrono.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>

#include <fmt/ostream.h>
#include <fmt/printf.h>

#include <cstdint>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
//[fibonacci
std::uint64_t fibonacci(std::uint64_t n)
{
    if (n < 2)
        return n;

    // Invoking the Fibonacci algorithm twice is inefficient.
    // However, we intentionally demonstrate it this way to create some
    // heavy workload.

    pika::future<std::uint64_t> n1 = pika::async(fibonacci, n - 1);
    pika::future<std::uint64_t> n2 = pika::async(fibonacci, n - 2);

    return n1.get() + n2.get();    // wait for the Futures to return their values
}
//fibonacci]

///////////////////////////////////////////////////////////////////////////////
//[pika_main
int pika_main(pika::program_options::variables_map& vm)
{
    // extract command line argument, i.e. fib(N)
    std::uint64_t n = vm["n-value"].as<std::uint64_t>();

    {
        // Keep track of the time required to execute.
        pika::chrono::detail::high_resolution_timer t;

        std::uint64_t r = fibonacci(n);

        constexpr char const* fmt = "fibonacci({}) == {}\nelapsed time: {} [s]\n";
        fmt::print(std::cout, fmt, n, r, t.elapsed<std::chrono::seconds>());
    }

    return pika::finalize();    // Handles pika shutdown
}
//pika_main]

///////////////////////////////////////////////////////////////////////////////
//[main
int main(int argc, char* argv[])
{
    // Configure application-specific options
    pika::program_options::options_description desc_commandline(
        "Usage: " PIKA_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("n-value",
        pika::program_options::value<std::uint64_t>()->default_value(10),
        "n value for the Fibonacci function");

    // Initialize and run pika
    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return pika::init(pika_main, argc, argv, init_args);
}
//main]
