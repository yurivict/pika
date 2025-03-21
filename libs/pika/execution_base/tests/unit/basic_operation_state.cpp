//  Copyright (c) 2020 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/execution_base/operation_state.hpp>
#include <pika/testing.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

bool start_called = false;

namespace mylib {
    struct state_1
    {
    };

    struct state_2
    {
        friend void tag_invoke(pika::execution::experimental::start_t, state_2&) {}
    };

    struct state_3
    {
        friend void tag_invoke(pika::execution::experimental::start_t, state_3&) noexcept
        {
            start_called = true;
        }
    };

    struct state_4
    {
    };

    void tag_invoke(pika::execution::experimental::start_t, state_4&) {}

    struct state_5
    {
    };

    void tag_invoke(pika::execution::experimental::start_t, state_5&) noexcept
    {
        start_called = true;
    }
}    // namespace mylib

int main()
{
    static_assert(!pika::execution::experimental::is_operation_state_v<mylib::state_1>,
        "mylib::state_1 is not an operation state");
    static_assert(!pika::execution::experimental::is_operation_state_v<mylib::state_2>,
        "mylib::state_2 is not an operation state");
    static_assert(pika::execution::experimental::is_operation_state_v<mylib::state_3>,
        "mylib::state_3 is an operation state");
    static_assert(!pika::execution::experimental::is_operation_state_v<mylib::state_4>,
        "mylib::state_4 is not an operation state");
    static_assert(pika::execution::experimental::is_operation_state_v<mylib::state_5>,
        "mylib::state_5 is an operation state");

    {
        mylib::state_3 state;

        pika::execution::experimental::start(state);
        PIKA_TEST(start_called);
        start_called = false;
    }
    {
        mylib::state_5 state;

        pika::execution::experimental::start(state);
        PIKA_TEST(start_called);
        start_called = false;
    }

    return 0;
}
