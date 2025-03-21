//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Parts of this code were inspired by https://github.com/josuttis/jthread. The
//  original code was published by Nicolai Josuttis and Lewis Baker under the
//  Creative Commons Attribution 4.0 International License
//  (http://creativecommons.org/licenses/by/4.0/).

#include <pika/init.hpp>
#include <pika/modules/synchronization.hpp>
#include <pika/testing.hpp>

#include <functional>
#include <type_traits>
#include <utility>

void test_stop_callback_inits()
{
    pika::stop_token token;

    struct implicit_arg
    {
    };
    struct explicit_arg
    {
    };

    struct my_callback
    {
        my_callback() {}
        my_callback(implicit_arg) {}
        explicit my_callback(explicit_arg) {}

        void operator()()
        {
            PIKA_TEST(false);
        }
    };

    auto stop10 = [] {};
    pika::stop_callback<decltype(stop10)> cb10{token, stop10};
    static_assert(std::is_same<decltype(cb10)::callback_type, decltype(stop10)>::value,
        "std::is_same<decltype(cb10)::callback_type, decltype(stop10)>::value");

    auto stop11 = [] { PIKA_TEST(false); };
    pika::stop_callback<decltype(stop11)> cb11{token, std::move(stop11)};
    static_assert(std::is_same<decltype(cb11)::callback_type, decltype(stop11)>::value,
        "std::is_same<decltype(cb11)::callback_type, decltype(stop11)>::value");
    static_assert(!std::is_reference<decltype(cb11)::callback_type>::value,
        "!std::is_reference<decltype(cb11)::callback_type>::value");

    auto stop12 = [] { PIKA_TEST(false); };
    pika::stop_callback<std::reference_wrapper<decltype(stop12)>> cb12{token, std::ref(stop12)};
    static_assert(std::is_same<decltype(cb12)::callback_type,
                      std::reference_wrapper<decltype(stop12)>>::value,
        "std::is_same<decltype(cb12)::callback_type, std::reference_wrapper<decltype(stop12) >> "
        "::value");
    static_assert(!std::is_reference<decltype(cb12)::callback_type>::value,
        "!std::is_reference<decltype(cb12)::callback_type>::value");

    auto cb13 = [] { PIKA_TEST(false); };
    pika::stop_callback<decltype(cb13)> scb13(token, std::move(cb13));
    static_assert(!std::is_reference<decltype(scb13)::callback_type>::value,
        "!std::is_reference<decltype(scb13)::callback_type>::value");

    pika::stop_callback<std::function<void()>> cb14{token, [] { PIKA_TEST(false); }};
    static_assert(std::is_same<decltype(cb14)::callback_type, std::function<void()>>::value,
        "std::is_same<decltype(cb14)::callback_type, std::function<void() >> ::value");
    static_assert(!std::is_reference<decltype(cb14)::callback_type>::value,
        "!std::is_reference<decltype(cb14)::callback_type>::value");

    std::function<void()> stop15 = [] { PIKA_TEST(false); };
    pika::stop_callback<decltype(stop15)> cb15(token, stop15);
    static_assert(std::is_same<decltype(cb15)::callback_type, std::function<void()>>::value,
        "std::is_same<decltype(cb15)::callback_type, std::function<void() >> ::value");
    static_assert(!std::is_reference<decltype(cb15)::callback_type>::value,
        "!std::is_reference<decltype(cb15)::callback_type>::value");

    std::function<void()> stop16 = [] { PIKA_TEST(false); };
    pika::stop_callback<std::function<void()>> cb16{token, stop16};
    static_assert(std::is_same<decltype(cb16)::callback_type, std::function<void()>>::value,
        "std::is_same<decltype(cb16)::callback_type, std::function<void() >> ::value");
    static_assert(!std::is_reference<decltype(cb16)::callback_type>::value,
        "!std::is_reference<decltype(cb16)::callback_type>::value");

    std::function<void()> stop17 = [token] {
        std::function<void()> f;
        if (true)
        {
            f = [] { PIKA_TEST(false); };
        }
        else
        {
            f = [] { PIKA_TEST(false); };
        }
        return f;
    }();
    pika::stop_callback<std::function<void()>> cb17(token, stop17);
    static_assert(std::is_same<decltype(cb17)::callback_type, std::function<void()>>::value,
        "std::is_same<decltype(cb17)::callback_type, std::function<void() >> ::value");
    static_assert(!std::is_reference<decltype(cb17)::callback_type>::value,
        "!std::is_reference<decltype(cb17)::callback_type>::value");

    implicit_arg i;
    pika::stop_callback<my_callback> cb18{token, i};
    static_assert(std::is_same<decltype(cb18)::callback_type, my_callback>::value,
        "std::is_same<decltype(cb18)::callback_type, my_callback>::value");
    static_assert(!std::is_reference<decltype(cb18)::callback_type>::value,
        "!std::is_reference<decltype(cb18)::callback_type>::value");

    explicit_arg e;
    pika::stop_callback<my_callback> cb19{token, e};
    static_assert(std::is_same<decltype(cb19)::callback_type, my_callback>::value,
        "std::is_same<decltype(cb19)::callback_type, my_callback>::value");
    static_assert(!std::is_reference<decltype(cb19)::callback_type>::value,
        "!std::is_reference<decltype(cb19)::callback_type>::value");

    // the following should fail compiling
    // pika::stop_callback<my_callback> cb =
    //   []() -> stop_callback<my_callback> {
    //       explicit_arg e;
    //       return {token, e};
    //   }();
    //
    // pika::stop_callback<my_callback> cb =
    //   []() -> stop_callback<my_callback> {
    //       implicit_arg i;
    //       return {token, i};
    //   }();
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    try
    {
        test_stop_callback_inits();
        // We expect to reach this point without an exception having been thrown
        PIKA_TEST(true);
    }
    catch (...)
    {
        PIKA_TEST(false);
    }
    pika::finalize();
    return 0;
}

int main(int argc, char* argv[])
{
    return pika::init(pika_main, argc, argv);
}
