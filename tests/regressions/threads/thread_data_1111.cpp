//  Copyright (c) 2014 Jeremy Kemp
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test illustrates #1111: pika::threads::detail::get_thread_data always returns zero

#include <pika/init.hpp>
#include <pika/testing.hpp>
#include <pika/thread.hpp>

#include <cstddef>
#include <iostream>
#include <memory>

struct thread_data
{
    int thread_num;
};

int get_thread_num()
{
    pika::threads::detail::thread_id_type thread_id = pika::threads::detail::get_self_id();
    thread_data* data =
        reinterpret_cast<thread_data*>(pika::threads::detail::get_thread_data(thread_id));
    PIKA_TEST(data);
    return data ? data->thread_num : 0;
}

int pika_main()
{
    std::unique_ptr<thread_data> data_struct(new thread_data());
    data_struct->thread_num = 42;

    pika::threads::detail::thread_id_type thread_id = pika::threads::detail::get_self_id();
    pika::threads::detail::set_thread_data(
        thread_id, reinterpret_cast<std::size_t>(data_struct.get()));

    PIKA_TEST_EQ(get_thread_num(), 42);

    return pika::finalize();
}

int main(int argc, char** argv)
{
    pika::init(pika_main, argc, argv);

    return 0;
}
