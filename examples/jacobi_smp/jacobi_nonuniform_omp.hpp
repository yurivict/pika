//  Copyright (c) 2011-2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/chrono.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "jacobi_nonuniform.hpp"

namespace jacobi_smp {
    void jacobi(crs_matrix<double> const& A, std::vector<double> const& b, std::size_t iterations,
        std::size_t block_size)
    {
        using vector_type = std::vector<double>;

        std::shared_ptr<vector_type> dst(new vector_type(b));
        std::shared_ptr<vector_type> src(new vector_type(b));

        pika::chrono::detail::high_resolution_timer t;
        for (std::size_t i = 0; i < iterations; ++i)
        {
            // MSVC is unhappy if the OMP loop variable is unsigned
#pragma omp parallel for schedule(JACOBI_SMP_OMP_SCHEDULE)
            for (std::int64_t row = 0; row < std::int64_t(b.size()); ++row)
            {
                jacobi_kernel_nonuniform(A, *dst, *src, b, row);
            }
            std::swap(dst, src);
        }

        double time_elapsed = t.elapsed<seconds>();
        std::cout << dst->size() << " " << ((double(dst->size() * iterations) / 1e6) / time_elapsed)
                  << " MLUPS/s\n"
                  << std::flush;
    }
}    // namespace jacobi_smp
