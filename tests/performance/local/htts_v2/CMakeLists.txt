# Copyright (c) 2011-2014 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(benchmarks htts2_payload_precision htts2_pika)

if(PIKA_WITH_EXAMPLES_OPENMP)
  set(benchmarks ${benchmarks} htts2_omp)
endif()

if(PIKA_WITH_EXAMPLES_QTHREADS)
  set(benchmarks ${benchmarks} htts2_qthreads)
  set(htts2_qthreads_LIBRARIES ${QTHREADS_LIBRARY})
  set(htts2_qthreads_INCLUDE_DIRECTORIES ${QTHREADS_INCLUDE_DIR})
endif()

if(PIKA_WITH_EXAMPLES_TBB)
  set(benchmarks ${benchmarks} htts2_tbb)
  set(htts2_tbb_LIBRARIES ${TBB_LIBRARY})
  set(htts2_tbb_INCLUDE_DIRECTORIES ${TBB_INCLUDE_DIR})
endif()

foreach(benchmark ${benchmarks})
  set(sources htts2.cpp ${benchmark}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  pika_add_executable(
    ${benchmark} INTERNAL_FLAGS
    SOURCES ${sources} ${${benchmark}_FLAGS}
    DEPENDENCIES ${${benchmark}_LIBRARIES}
    EXCLUDE_FROM_ALL
    FOLDER "Benchmarks/HTTS v2"
  )

  target_include_directories(
    ${benchmark} SYSTEM PRIVATE ${${benchmark}_INCLUDE_DIRECTORIES}
  )

  # add a custom target for this example
  pika_add_pseudo_target(tests.performance.local.htts_v2.${benchmark})

  # make pseudo-targets depend on master pseudo-target
  pika_add_pseudo_dependencies(
    tests.performance.local.htts_v2
    tests.performance.local.htts_v2.${benchmark}
  )

  # add dependencies to pseudo-target
  pika_add_pseudo_dependencies(
    tests.performance.local.htts_v2.${benchmark} ${benchmark}
  )
endforeach()

if(PIKA_WITH_EXAMPLES_OPENMP)
  set_target_properties(htts2_omp PROPERTIES COMPILE_FLAGS ${OpenMP_CXX_FLAGS})
  set_target_properties(htts2_omp PROPERTIES LINK_FLAGS ${OpenMP_CXX_FLAGS})
endif()
