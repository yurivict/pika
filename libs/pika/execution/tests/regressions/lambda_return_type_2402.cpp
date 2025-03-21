//  Copyright (c) 2016 David Pfander
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/algorithm.hpp>
#include <pika/init.hpp>
#include <pika/modules/iterator_support.hpp>
#include <pika/parallel/datapar.hpp>
#include <pika/testing.hpp>

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <vector>

int pika_main()
{
    std::vector<double> large(64);

    auto zip_it_begin = pika::util::make_zip_iterator(large.begin());
    auto zip_it_end = pika::util::make_zip_iterator(large.end());

    pika::for_each(pika::execution::par_simd, zip_it_begin, zip_it_end, [](auto t) {
        using comp_type = typename std::tuple_element<0, decltype(t)>::type;
        using var_type = std::decay_t<comp_type>;

        var_type mass_density = 0.0;
        mass_density(mass_density > 0.0) = 7.0;

        PIKA_TEST(all_of(mass_density == 0.0));
    });

    return pika::finalize();    // Handles pika shutdown
}

int main(int argc, char** argv)
{
    PIKA_TEST_EQ(pika::init(pika_main, argc, argv), 0);
    return 0;
}
