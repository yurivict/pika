//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/init.hpp>
#include <pika/runtime/config_entry.hpp>
#include <pika/string_util/from_string.hpp>
#include <pika/testing.hpp>

#include <atomic>
#include <string>

void test_get_entry()
{
    std::string val = pika::get_config_entry("pika.localities", "42");
    PIKA_TEST(!val.empty());
    PIKA_TEST_EQ(pika::detail::from_string<int>(val), 1);

    val = pika::get_config_entry("pika.localities", 42);
    PIKA_TEST(!val.empty());
    PIKA_TEST_EQ(pika::detail::from_string<int>(val), 1);
}

std::atomic<bool> invoked_callback(false);

void config_entry_callback(std::string const& key, std::string const& val)
{
    PIKA_TEST_EQ(key, std::string("pika.config.entry.test"));
    PIKA_TEST_EQ(val, std::string("test1"));

    PIKA_TEST(!invoked_callback.load());
    invoked_callback = true;
}

void test_set_entry()
{
    std::string val = pika::get_config_entry("pika.config.entry.test", "");
    PIKA_TEST(val.empty());

    pika::set_config_entry("pika.config.entry.test", "test");
    val = pika::get_config_entry("pika.config.entry.test", "");
    PIKA_TEST(!val.empty());
    PIKA_TEST_EQ(val, std::string("test"));

    pika::set_config_entry_callback("pika.config.entry.test", &config_entry_callback);

    pika::set_config_entry("pika.config.entry.test", "test1");
    val = pika::get_config_entry("pika.config.entry.test", "");
    PIKA_TEST(!val.empty());
    PIKA_TEST_EQ(val, std::string("test1"));

    PIKA_TEST(invoked_callback.load());
}

int pika_main()
{
    test_get_entry();
    test_set_entry();
    return pika::finalize();
}

int main(int argc, char** argv)
{
    pika::init(pika_main, argc, argv);

    return 0;
}
