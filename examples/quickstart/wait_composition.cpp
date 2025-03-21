////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <pika/future.hpp>
#include <pika/init.hpp>

#include <iostream>
#include <tuple>

///////////////////////////////////////////////////////////////////////////////
struct cout_continuation
{
    using data_type = std::tuple<pika::future<int>, pika::future<int>, pika::future<int>>;

    void operator()(pika::future<data_type> data) const
    {
        data_type v = data.get();
        std::cout << std::get<0>(v).get() << "\n";
        std::cout << std::get<1>(v).get() << "\n";
        std::cout << std::get<2>(v).get() << "\n";
    }
};
///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    {
        pika::future<int> a = pika::make_ready_future<int>(17);
        pika::future<int> b = pika::make_ready_future<int>(42);
        pika::future<int> c = pika::make_ready_future<int>(-1);

        pika::when_all(a, b, c).then(cout_continuation());
    }

    return pika::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    return pika::init(pika_main, argc, argv);    // Initialize and run pika.
}
