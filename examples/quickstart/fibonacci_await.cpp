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
#include <pika/functional.hpp>
#include <pika/future.hpp>
#include <pika/init.hpp>

#include <fmt/ostream.h>
#include <fmt/printf.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
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
//
// pika::future<std::uint64_t> fibonacci(std::uint64_t) resumable
// {
//     if (n < 2) return pika::make_ready_future(n);
//     if (n < threshold) return pika::make_ready_future(fibonacci_serial(n));
//
//     pika::future<std::uint64_t> lhs = pika::async(&fibonacci, n-1);
//     pika::future<std::uint64_t> rhs = fibonacci(n-2);
//
//     return await lhs + await rhs;
// }
//

pika::future<std::uint64_t> fibonacci(std::uint64_t n);

struct fibonacci_frame
{
    int state_;
    pika::future<std::uint64_t> result_;
    pika::lcos::local::promise<std::uint64_t> result_promise_;

    fibonacci_frame(std::uint64_t n)
      : state_(0)
      , n_(n)
      , lhs_result_(0)
      , rhs_result_(0)
    {
    }

    // local variables
    std::uint64_t n_;
    pika::future<std::uint64_t> lhs_;
    pika::future<std::uint64_t> rhs_;
    std::uint64_t lhs_result_;
    std::uint64_t rhs_result_;
};

void fibonacci_impl(std::shared_ptr<fibonacci_frame> const& frame_)
{
    fibonacci_frame* frame = frame_.get();
    int state = frame->state_;

    switch (state)
    {
    case 1:
        goto L1;
    case 2:
        goto L2;
    }

    // if (n < 2) return pika::make_ready_future(n);
    if (frame->n_ < 2)
    {
        if (state == 0)
            // never paused
            frame->result_ = pika::make_ready_future(frame->n_);
        else
            frame->result_promise_.set_value(frame->n_);
        return;
    }

    // if (n < threshold) return pika::make_ready_future(fibonacci_serial(n));
    if (frame->n_ < threshold)
    {
        if (state == 0)
            // never paused
            frame->result_ = pika::make_ready_future(fibonacci_serial(frame->n_));
        else
            frame->result_promise_.set_value(fibonacci_serial(frame->n_));
        return;
    }

    // pika::future<std::uint64_t> lhs = pika::async(&fibonacci, n-1);
    frame->lhs_ = pika::async(&fibonacci, frame->n_ - 1);

    // pika::future<std::uint64_t> rhs = fibonacci(n-2);
    frame->rhs_ = fibonacci(frame->n_ - 2);

    if (!frame->lhs_.is_ready())
    {
        frame->state_ = 1;
        if (!frame->result_.valid())
            frame->result_ = frame->result_promise_.get_future();
        frame->lhs_.then(pika::util::detail::bind(&fibonacci_impl, frame_));
        return;
    }

L1:
    frame->lhs_result_ = frame->lhs_.get();

    if (!frame->rhs_.is_ready())
    {
        frame->state_ = 2;
        if (!frame->result_.valid())
            frame->result_ = frame->result_promise_.get_future();
        frame->rhs_.then(pika::util::detail::bind(&fibonacci_impl, frame_));
        return;
    }

L2:
    frame->rhs_result_ = frame->rhs_.get();

    if (state == 0)
        // never paused
        frame->result_ = pika::make_ready_future(frame->lhs_result_ + frame->rhs_result_);
    else
        frame->result_promise_.set_value(frame->lhs_result_ + frame->rhs_result_);
    return;
}

pika::future<std::uint64_t> fibonacci(std::uint64_t n)
{
    std::shared_ptr<fibonacci_frame> frame = std::make_shared<fibonacci_frame>(n);

    fibonacci_impl(frame);

    return std::move(frame->result_);
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
        std::cerr << "fibonacci_await: wrong command line argument value for option 'n-runs', "
                     "should not be zero"
                  << std::endl;
        return -1;
    }

    threshold = vm["threshold"].as<unsigned int>();
    if (threshold < 2 || threshold > n)
    {
        std::cerr << "fibonacci_await: wrong command line argument value for option 'threshold', "
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
            // serial execution
            r = fibonacci_serial(n);
        }

        std::uint64_t d = duration_cast<seconds>(high_resolution_clock::now() - start).count();
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
            // Create a future for the whole calculation, execute it locally,
            // and wait for it.
            r = fibonacci(n).get();
        }

        std::uint64_t d = duration_cast<seconds>(high_resolution_clock::now() - start).count();
        constexpr char const* fmt = "fibonacci_await({}) == {},elapsed time:,{},[s]\n";
        fmt::print(std::cout, fmt, n, r, d / max_runs);

        executed_one = true;
    }

    if (!executed_one)
    {
        std::cerr << "fibonacci_await: wrong command line argument value for option 'tests', "
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
