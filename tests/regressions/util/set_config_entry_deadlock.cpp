//  Copyright (c) 2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#if !defined(PIKA_COMPUTE_DEVICE_CODE)
# include <pika/functional/bind.hpp>
# include <pika/init.hpp>
# include <pika/runtime/config_entry.hpp>
# include <pika/testing.hpp>

# include <atomic>
# include <string>

std::atomic<bool> invoked_callback(false);

void config_entry_callback()
{
    // this used to cause a deadlock in the config registry
    std::string val = pika::get_config_entry("pika.config.entry.test", "");
    PIKA_TEST_EQ(val, std::string("test1"));

    PIKA_TEST(!invoked_callback.load());
    invoked_callback = true;
}

int pika_main()
{
    std::string val = pika::get_config_entry("pika.config.entry.test", "");
    PIKA_TEST(val.empty());

    pika::set_config_entry("pika.config.entry.test", "test");
    val = pika::get_config_entry("pika.config.entry.test", "");
    PIKA_TEST(!val.empty());
    PIKA_TEST_EQ(val, std::string("test"));

    pika::set_config_entry_callback(
        "pika.config.entry.test", pika::util::detail::bind(&config_entry_callback));

    pika::set_config_entry("pika.config.entry.test", "test1");
    PIKA_TEST(invoked_callback.load());

    val = pika::get_config_entry("pika.config.entry.test", "");
    PIKA_TEST(!val.empty());
    PIKA_TEST_EQ(val, std::string("test1"));

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    PIKA_TEST_EQ(pika::init(pika_main, argc, argv), 0);
    return 0;
}
#endif
