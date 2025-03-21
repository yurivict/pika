//  Copyright (c) 2007-2013 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is a purely local version demonstrating different versions of making
// the calculation of a fibonacci asynchronous.

#include <pika/chrono.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>

#include <fmt/ostream.h>
#include <fmt/printf.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
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
std::uint64_t add(pika::future<std::uint64_t> f1, pika::future<std::uint64_t> f2)
{
    return f1.get() + f2.get();
}

///////////////////////////////////////////////////////////////////////////////
struct when_all_wrapper
{
    using data_type = std::tuple<pika::future<std::uint64_t>, pika::future<std::uint64_t>>;

    std::uint64_t operator()(pika::future<data_type> data) const
    {
        data_type v = data.get();
        return std::get<0>(v).get() + std::get<1>(v).get();
    }
};

///////////////////////////////////////////////////////////////////////////////
pika::future<std::uint64_t> fibonacci_future_one(std::uint64_t n);

struct fibonacci_future_one_continuation
{
    explicit fibonacci_future_one_continuation(std::uint64_t n)
      : n_(n)
    {
    }

    std::uint64_t operator()(pika::future<std::uint64_t> res) const
    {
        return add(fibonacci_future_one(n_ - 2), std::move(res));
    }

    std::uint64_t n_;
};

std::uint64_t fib(std::uint64_t n)
{
    return fibonacci_future_one(n).get();
}

pika::future<std::uint64_t> fibonacci_future_one(std::uint64_t n)
{
    // if we know the answer, we return a future encapsulating the final value
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    // asynchronously launch the calculation of one of the sub-terms
    // attach a continuation to this future which is called asynchronously on
    // its completion and which calculates the other sub-term
    return pika::async(&fib, n - 1).then(fibonacci_future_one_continuation(n));
}

///////////////////////////////////////////////////////////////////////////////
std::uint64_t fibonacci(std::uint64_t n)
{
    // if we know the answer, we return the final value
    if (n < 2)
        return n;
    if (n < threshold)
        return fibonacci_serial(n);

    // asynchronously launch the creation of one of the sub-terms of the
    // execution graph
    pika::future<std::uint64_t> f = pika::async(&fibonacci, n - 1);
    std::uint64_t r = fibonacci(n - 2);

    return f.get() + r;
}

///////////////////////////////////////////////////////////////////////////////
std::uint64_t fibonacci_fork(std::uint64_t n)
{
    // if we know the answer, we return the final value
    if (n < 2)
        return n;
    if (n < threshold)
        return fibonacci_serial(n);

    // asynchronously launch the creation of one of the sub-terms of the
    // execution graph
    pika::future<std::uint64_t> f = pika::async(pika::launch::fork, &fibonacci_fork, n - 1);
    std::uint64_t r = fibonacci_fork(n - 2);

    return f.get() + r;
}

///////////////////////////////////////////////////////////////////////////////
pika::future<std::uint64_t> fibonacci_future(std::uint64_t n)
{
    // if we know the answer, we return a future encapsulating the final value
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    // asynchronously launch the creation of one of the sub-terms of the
    // execution graph
    pika::future<std::uint64_t> f = pika::async(&fibonacci_future, n - 1);
    pika::future<std::uint64_t> r = fibonacci_future(n - 2);

    return pika::async(&add, std::move(f), std::move(r));
}

///////////////////////////////////////////////////////////////////////////////
pika::future<std::uint64_t> fibonacci_future_fork(std::uint64_t n)
{
    // if we know the answer, we return a future encapsulating the final value
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    // asynchronously launch the creation of one of the sub-terms of the
    // execution graph
    pika::future<std::uint64_t> f = pika::async(pika::launch::fork, &fibonacci_future_fork, n - 1);
    pika::future<std::uint64_t> r = fibonacci_future_fork(n - 2);

    return pika::async(&add, std::move(f), std::move(r));
}

///////////////////////////////////////////////////////////////////////////////
pika::future<std::uint64_t> fibonacci_future_when_all(std::uint64_t n)
{
    // if we know the answer, we return a future encapsulating the final value
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    // asynchronously launch the creation of one of the sub-terms of the
    // execution graph
    pika::future<pika::future<std::uint64_t>> f = pika::async(&fibonacci_future, n - 1);
    pika::future<std::uint64_t> r = fibonacci_future(n - 2);

    return pika::when_all(f.get(), r).then(when_all_wrapper());
}

pika::future<std::uint64_t> fibonacci_future_unwrapped_when_all(std::uint64_t n)
{
    // if we know the answer, we return a future encapsulating the final value
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    // asynchronously launch the creation of one of the sub-terms of the
    // execution graph
    pika::future<std::uint64_t> f = pika::async(&fibonacci_future, n - 1);
    pika::future<std::uint64_t> r = fibonacci_future(n - 2);

    return pika::when_all(f, r).then(when_all_wrapper());
}

/////////////////////////////////////////////////////////////////////////////
pika::future<std::uint64_t> fibonacci_future_all(std::uint64_t n)
{
    // if we know the answer, we return a future encapsulating the final value
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    // asynchronously launch the calculation of both of the sub-terms
    pika::future<std::uint64_t> f1 = fibonacci_future_all(n - 1);
    pika::future<std::uint64_t> f2 = fibonacci_future_all(n - 2);

    // create a future representing the successful calculation of both sub-terms
    return pika::async(&add, std::move(f1), std::move(f2));
}

/////////////////////////////////////////////////////////////////////////////
pika::future<std::uint64_t> fibonacci_future_all_when_all(std::uint64_t n)
{
    // if we know the answer, we return a future encapsulating the final value
    if (n < 2)
        return pika::make_ready_future(n);
    if (n < threshold)
        return pika::make_ready_future(fibonacci_serial(n));

    // asynchronously launch the calculation of both of the sub-terms
    pika::future<std::uint64_t> f1 = fibonacci_future_all(n - 1);
    pika::future<std::uint64_t> f2 = fibonacci_future_all(n - 2);

    // create a future representing the successful calculation of both sub-terms
    // attach a continuation to this future which is called asynchronously on
    // its completion and which calculates the final result
    return pika::when_all(f1, f2).then(when_all_wrapper());
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
        std::cerr << "fibonacci_futures: wrong command line argument value for option 'n-runs', "
                     "should not be zero"
                  << std::endl;
        return -1;
    }

    threshold = vm["threshold"].as<unsigned int>();
    if (threshold < 2 || threshold > n)
    {
        std::cerr << "fibonacci_futures: wrong command line argument value for option 'threshold', "
                     "should be in between 2 and n-value, value specified: "
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

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally,
            // and wait for it.
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

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally,
            // and wait for it.
            r = fibonacci_future_one(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_future_one({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "2")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally, and
            // wait for it.
            r = fibonacci(n);
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "9")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally, and
            // wait for it. Use continuation stealing
            r = fibonacci_fork(n);
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_fork({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "3")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally, and
            // wait for it.
            r = fibonacci_future(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_future({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "8")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally, and
            // wait for it. Use continuation stealing.
            r = fibonacci_future_fork(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_future_fork({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "6")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally, and
            // wait for it.
            r = fibonacci_future_when_all(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_future_when_all({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "7")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally, and
            // wait for it.
            r = fibonacci_future_unwrapped_when_all(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt =
            "fibonacci_future_unwrapped_when_all({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "4")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a future for the whole calculation, execute it locally, and
            // wait for it.
            r = fibonacci_future_all(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_future_all({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (test == "all" || test == "5")
    {
        // Keep track of the time required to execute.
        auto start = high_resolution_clock::now();

        for (std::size_t i = 0; i != max_runs; ++i)
        {
            // Create a Future for the whole calculation, execute it locally, and
            // wait for it.
            r = fibonacci_future_all_when_all(n).get();
        }

        std::uint64_t d = duration_cast<nanoseconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt =
            "fibonacci_future_all_when_all({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (!executed_one)
    {
        std::cerr << "fibonacci_futures: wrong command line argument value for option 'tests', "
                     "should be either 'all' or a number between zero and 7, value specified: "
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
        "select tests to execute (0-9, default: all)");
    // clang-format on

    // Initialize and run pika
    pika::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return pika::init(pika_main, argc, argv, init_args);
}
