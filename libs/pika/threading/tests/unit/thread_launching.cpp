// Copyright (C) 2012 Hartmut Kaiser
// Copyright (C) 2007-8 Anthony Williams
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/init.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <cmath>
#include <functional>
#include <string>
#include <vector>

using pika::program_options::options_description;
using pika::program_options::variables_map;

///////////////////////////////////////////////////////////////////////////////
bool normal_function_called = false;

void normal_function()
{
    normal_function_called = true;
}

void test_thread_function_no_arguments()
{
    pika::thread function(&normal_function);
    function.join();
    PIKA_TEST(normal_function_called);
}

///////////////////////////////////////////////////////////////////////////////
int nfoa_res = 0;

void normal_function_one_arg(int i)
{
    nfoa_res = i;
}

void test_thread_function_one_argument()
{
    pika::thread function(&normal_function_one_arg, 42);
    function.join();
    PIKA_TEST_EQ(42, nfoa_res);
}

///////////////////////////////////////////////////////////////////////////////
struct callable_no_args
{
    static bool called;

    void operator()() const
    {
        called = true;
    }
};

bool callable_no_args::called = false;

void test_thread_callable_object_no_arguments()
{
    callable_no_args func;
    pika::thread callable(func);
    callable.join();
    PIKA_TEST(callable_no_args::called);
}

///////////////////////////////////////////////////////////////////////////////
struct callable_noncopyable_no_args
{
    static bool called;

    callable_noncopyable_no_args() {}

    callable_noncopyable_no_args(callable_noncopyable_no_args const&) = delete;
    callable_noncopyable_no_args& operator=(callable_noncopyable_no_args const&) = delete;

    void operator()() const
    {
        called = true;
    }
};

bool callable_noncopyable_no_args::called = false;

void test_thread_callable_object_ref_no_arguments()
{
    callable_noncopyable_no_args func;

    pika::thread callable(std::ref(func));
    callable.join();
    PIKA_TEST(callable_noncopyable_no_args::called);
}

///////////////////////////////////////////////////////////////////////////////
struct callable_one_arg
{
    static bool called;
    static int called_arg;

    void operator()(int arg) const
    {
        called = true;
        called_arg = arg;
    }
};

bool callable_one_arg::called = false;
int callable_one_arg::called_arg = 0;

void test_thread_callable_object_one_argument()
{
    callable_one_arg func;
    pika::thread callable(func, 42);
    callable.join();
    PIKA_TEST(callable_one_arg::called);
    PIKA_TEST_EQ(callable_one_arg::called_arg, 42);
}

///////////////////////////////////////////////////////////////////////////////
struct callable_multiple_arg
{
    static bool called_two;
    static int called_two_arg1;
    static double called_two_arg2;
    static bool called_three;
    static std::string called_three_arg1;
    static std::vector<int> called_three_arg2;
    static int called_three_arg3;

    void operator()(int arg1, double arg2) const
    {
        called_two = true;
        called_two_arg1 = arg1;
        called_two_arg2 = arg2;
    }
    void operator()(std::string const& arg1, std::vector<int> const& arg2, int arg3) const
    {
        called_three = true;
        called_three_arg1 = arg1;
        called_three_arg2 = arg2;
        called_three_arg3 = arg3;
    }
};

bool callable_multiple_arg::called_two = false;
bool callable_multiple_arg::called_three = false;
int callable_multiple_arg::called_two_arg1;
double callable_multiple_arg::called_two_arg2;
std::string callable_multiple_arg::called_three_arg1;
std::vector<int> callable_multiple_arg::called_three_arg2;
int callable_multiple_arg::called_three_arg3;

void test_thread_callable_object_multiple_arguments()
{
    std::vector<int> x;
    for (int i = 0; i < 7; ++i)
    {
        x.push_back(i * i);
    }

    callable_multiple_arg func;

    pika::thread callable3(func, "hello", x, 1.2);
    callable3.join();
    PIKA_TEST(callable_multiple_arg::called_three);
    PIKA_TEST_EQ(callable_multiple_arg::called_three_arg1, "hello");
    PIKA_TEST_EQ(callable_multiple_arg::called_three_arg2.size(), x.size());
    for (unsigned j = 0; j < x.size(); ++j)
    {
        PIKA_TEST_EQ(callable_multiple_arg::called_three_arg2.at(j), x[j]);
    }

    PIKA_TEST_EQ(callable_multiple_arg::called_three_arg3, 1);

    double const dbl = 1.234;

    pika::thread callable2(func, 19, dbl);
    callable2.join();
    PIKA_TEST(callable_multiple_arg::called_two);
    PIKA_TEST_LT(std::abs(callable_multiple_arg::called_two_arg1 - 19.), 1e-16);
    PIKA_TEST_LT(std::abs(callable_multiple_arg::called_two_arg2 - dbl), 1e-16);
}

///////////////////////////////////////////////////////////////////////////////
struct X
{
    bool function_called;
    int arg_value;

    X()
      : function_called(false)
      , arg_value(0)
    {
    }

    void f0()
    {
        function_called = true;
    }

    void f1(int i)
    {
        arg_value = i;
    }
};

void test_thread_member_function_no_arguments()
{
    X x;

    pika::thread function(&X::f0, &x);
    function.join();
    PIKA_TEST(x.function_called);
}

void test_thread_member_function_one_argument()
{
    X x;
    pika::thread function(&X::f1, &x, 42);
    function.join();
    PIKA_TEST_EQ(42, x.arg_value);
}

///////////////////////////////////////////////////////////////////////////////
int pika_main(variables_map&)
{
    {
        test_thread_function_no_arguments();
        test_thread_function_one_argument();
        test_thread_callable_object_no_arguments();
        test_thread_callable_object_ref_no_arguments();
        test_thread_callable_object_one_argument();
        test_thread_callable_object_multiple_arguments();
        test_thread_member_function_no_arguments();
        test_thread_member_function_one_argument();
    }

    pika::finalize();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    options_description cmdline("Usage: " PIKA_APPLICATION_STRING " [options]");

    // Initialize and run pika
    pika::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return pika::init(pika_main, argc, argv, init_args);
}
