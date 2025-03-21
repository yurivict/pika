# Copyright (c) 2007-2017 Hartmut Kaiser
# Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests split_future_2246 when_all_vectors_1623)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  pika_add_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Regressions/Modules/AsyncCombinators"
  )

  pika_add_regression_test(
    "modules.async_combinators" ${test} ${${test}_PARAMETERS}
  )
endforeach()

if(PIKA_WITH_COMPILE_ONLY_TESTS)
  set(compile_tests wait_all_std_array_2035)

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    pika_add_regression_compile_test(
      "modules.async_combinators" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Regressions/Modules/AsyncCombinators/CompileOnly"
    )
  endforeach()
endif()
