//  Copyright (c) 2020 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/functional/tag_invoke.hpp>
#include <pika/testing.hpp>

#include <type_traits>
#include <utility>

namespace mylib {
    inline constexpr struct foo_fn
    {
        // We use std::declval<foo_fn> in the function signature instead of
        // *this to work around a bug in GCC <= 8.
        template <typename T>
        constexpr auto operator()(T&& x) const noexcept(noexcept(
            pika::functional::detail::tag_invoke(std::declval<foo_fn>(), std::forward<T>(x))))
            -> decltype(pika::functional::detail::tag_invoke(
                std::declval<foo_fn>(), std::forward<T>(x)))
        {
            return pika::functional::detail::tag_invoke(*this, std::forward<T>(x));
        }
    } foo;

    inline constexpr struct bar_fn
    {
        // See above for an explanation of std::declval<bar_fn>().
        template <typename T, typename U>
        constexpr auto operator()(T&& x, U&& u) const
            noexcept(noexcept(pika::functional::detail::tag_invoke(
                std::declval<bar_fn>(), std::forward<T>(x), std::forward<U>(u))))
                -> decltype(pika::functional::detail::tag_invoke(
                    std::declval<bar_fn>(), std::forward<T>(x), std::forward<U>(u)))
        {
            return pika::functional::detail::tag_invoke(
                *this, std::forward<T>(x), std::forward<U>(u));
        }
    } bar;

    struct tag_invocable
    {
        friend constexpr bool tag_invoke(foo_fn, tag_invocable)
        {
            return true;
        }
    };

    struct tag_invocable2
    {
        friend constexpr bool tag_invoke(foo_fn, tag_invocable, int)
        {
            return true;
        }
    };

    struct tag_invocable_noexcept
    {
        friend constexpr bool tag_invoke(foo_fn, tag_invocable_noexcept) noexcept
        {
            return false;
        }
    };

    struct tag_not_invocable
    {
    };
}    // namespace mylib

namespace otherlib {
    struct tag_invocable
    {
        friend constexpr bool tag_invoke(mylib::foo_fn, tag_invocable)
        {
            return true;
        }
    };

    struct tag_invocable_noexcept
    {
        friend constexpr bool tag_invoke(mylib::foo_fn, tag_invocable_noexcept) noexcept
        {
            return false;
        }
    };

    struct tag_not_invocable
    {
    };
}    // namespace otherlib

namespace testlib {
    struct tag_invocable
    {
        friend constexpr int tag_invoke(mylib::foo_fn, tag_invocable const&)
        {
            return 0;
        }
        friend constexpr int tag_invoke(mylib::foo_fn, tag_invocable&)
        {
            return 1;
        }
        friend constexpr int tag_invoke(mylib::foo_fn, tag_invocable const&&)
        {
            return 2;
        }
        friend constexpr int tag_invoke(mylib::foo_fn, tag_invocable&&)
        {
            return 3;
        }
    };

    struct tag_invocable2
    {
        template <typename T>
        friend constexpr auto tag_invoke(mylib::bar_fn, tag_invocable2, T&& t) -> decltype(t)
        {
            return std::forward<T>(t);
        }
    };
}    // namespace testlib

int main()
{
    // Check if is_invocable trait works
    static_assert(pika::functional::detail::is_tag_invocable_v<mylib::foo_fn, mylib::tag_invocable>,
        "Should be tag invocable");
    static_assert(
        pika::functional::detail::is_tag_invocable_v<mylib::foo_fn, mylib::tag_invocable_noexcept>,
        "Should be tag invocable");
    static_assert(
        !pika::functional::detail::is_tag_invocable_v<mylib::foo_fn, mylib::tag_not_invocable>,
        "Should not be tag invocable");
    static_assert(
        !pika::functional::detail::is_tag_invocable_v<mylib::foo_fn, mylib::tag_invocable2>,
        "Should not be tag invocable");

    static_assert(
        pika::functional::detail::is_tag_invocable_v<mylib::foo_fn, otherlib::tag_invocable>,
        "Should be tag invocable");
    static_assert(pika::functional::detail::is_tag_invocable_v<mylib::foo_fn,
                      otherlib::tag_invocable_noexcept>,
        "Should be tag invocable");
    static_assert(
        !pika::functional::detail::is_tag_invocable_v<mylib::foo_fn, otherlib::tag_not_invocable>,
        "Should not be tag invocable");

    // Check if is_nothrow_invocable trait works
    static_assert(
        !pika::functional::detail::is_nothrow_tag_invocable_v<mylib::foo_fn, mylib::tag_invocable>,
        "Should not be nothrow tag invocable");
    static_assert(pika::functional::detail::is_nothrow_tag_invocable_v<mylib::foo_fn,
                      mylib::tag_invocable_noexcept>,
        "Should be nothrow tag invocable");
    static_assert(!pika::functional::detail::is_nothrow_tag_invocable_v<mylib::foo_fn,
                      mylib::tag_not_invocable>,
        "Should not be nothrow tag invocable");
    static_assert(
        !pika::functional::detail::is_nothrow_tag_invocable_v<mylib::foo_fn, mylib::tag_invocable2>,
        "Should not be nothrow tag invocable");

    static_assert(!pika::functional::detail::is_nothrow_tag_invocable_v<mylib::foo_fn,
                      otherlib::tag_invocable>,
        "Should not be nothrow tag invocable");
    static_assert(pika::functional::detail::is_nothrow_tag_invocable_v<mylib::foo_fn,
                      otherlib::tag_invocable_noexcept>,
        "Should be nothrow tag invocable");
    static_assert(!pika::functional::detail::is_nothrow_tag_invocable_v<mylib::foo_fn,
                      otherlib::tag_not_invocable>,
        "Should not be nothrow tag invocable");

    // Check if we properly propagate type categories
    constexpr testlib::tag_invocable dut0;
    static_assert(mylib::foo(dut0) == 0, "Needs to call the const ref overload");
    static_assert(mylib::foo(std::move(dut0)) == 2, "Needs to call the const ref overload");

    testlib::tag_invocable dut1;
    // Needs to call the non-const ref overload
    PIKA_TEST_EQ(mylib::foo(dut1), 1);
    // Needs to call the rvalue ref overload
    PIKA_TEST_EQ(mylib::foo(std::move(dut1)), 3);

    static_assert(
        pika::functional::detail::is_tag_invocable_v<mylib::bar_fn, testlib::tag_invocable2, int>,
        "Should be tag invocable");
    static_assert(
        pika::functional::detail::is_tag_invocable_v<mylib::bar_fn, testlib::tag_invocable2, int&>,
        "Should be tag invocable");
    static_assert(pika::functional::detail::is_tag_invocable_v<mylib::bar_fn,
                      testlib::tag_invocable2, int const&>,
        "Should be tag invocable");
    static_assert(pika::functional::detail::is_tag_invocable_v<mylib::bar_fn,
                      testlib::tag_invocable2, int const&&>,
        "Should be tag invocable");
    static_assert(
        pika::functional::detail::is_tag_invocable_v<mylib::bar_fn, testlib::tag_invocable2, int&&>,
        "Should be tag invocable");
    static_assert(
        !pika::functional::detail::is_tag_invocable_v<mylib::bar_fn, testlib::tag_invocable2>,
        "Should not be tag invocable");
    static_assert(!pika::functional::detail::is_tag_invocable_v<mylib::bar_fn,
                      testlib::tag_invocable2, int, int>,
        "Should not be tag invocable");

    int i = 0;
    PIKA_TEST_EQ(&mylib::bar(testlib::tag_invocable2{}, i), &i);
    static_assert(std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                                   testlib::tag_invocable2, int>,
                      int&&>::value,
        "Result type needs to match");
    static_assert(std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                                   testlib::tag_invocable2, int const&>,
                      int const&>::value,
        "Result type needs to match");
    static_assert(std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                                   testlib::tag_invocable2, int&>,
                      int&>::value,
        "Result type needs to match");
    static_assert(std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                                   testlib::tag_invocable2, int const&&>,
                      int const&&>::value,
        "Result type needs to match");
    static_assert(std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                                   testlib::tag_invocable2, int&&>,
                      int&&>::value,
        "Result type needs to match");
    static_assert(std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                                   testlib::tag_invocable2, int>,
                      decltype(mylib::bar(testlib::tag_invocable2{}, std::declval<int>()))>::value,
        "Result type needs to match");
    static_assert(
        std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                         testlib::tag_invocable2, int const&>,
            decltype(mylib::bar(testlib::tag_invocable2{}, std::declval<int const&>()))>::value,
        "Result type needs to match");
    static_assert(std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                                   testlib::tag_invocable2, int&>,
                      decltype(mylib::bar(testlib::tag_invocable2{}, std::declval<int&>()))>::value,
        "Result type needs to match");
    static_assert(
        std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                         testlib::tag_invocable2, int const&&>,
            decltype(mylib::bar(testlib::tag_invocable2{}, std::declval<int const&&>()))>::value,
        "Result type needs to match");
    static_assert(
        std::is_same<pika::functional::detail::tag_invoke_result_t<mylib::bar_fn,
                         testlib::tag_invocable2, int&&>,
            decltype(mylib::bar(testlib::tag_invocable2{}, std::declval<int&&>()))>::value,
        "Result type needs to match");
    static_assert(mylib::bar(testlib::tag_invocable2{}, 42) == 42,
        "This function should be constexpr evaluated");

    return 0;
}
