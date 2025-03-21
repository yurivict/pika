//  Copyright (c) 2018 Mikael Simberg
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example creates a resource partitioner, a custom thread pool, and adds
// processing units from a single NUMA domain to the custom thread pool. It is
// intended for inclusion in the documentation.

//[body
#include <pika/init.hpp>
#include <pika/modules/resource_partitioner.hpp>

#include <iostream>

int pika_main()
{
    return pika::finalize();
}

void init_resource_partitioner_handler(
    pika::resource::partitioner& rp, pika::program_options::variables_map const& /*vm*/)
{
    rp.create_thread_pool("my-thread-pool");

    bool one_numa_domain = rp.numa_domains().size() == 1;
    bool skipped_first_pu = false;

    pika::resource::numa_domain const& d = rp.numa_domains()[0];

    for (pika::resource::core const& c : d.cores())
    {
        for (pika::resource::pu const& p : c.pus())
        {
            if (one_numa_domain && !skipped_first_pu)
            {
                skipped_first_pu = true;
                continue;
            }

            rp.add_resource(p, "my-thread-pool");
        }
    }
}

int main(int argc, char* argv[])
{
    // Set the callback to init the thread_pools
    pika::init_params init_args;
    init_args.rp_callback = &init_resource_partitioner_handler;

    pika::init(pika_main, argc, argv, init_args);
}
//body]
