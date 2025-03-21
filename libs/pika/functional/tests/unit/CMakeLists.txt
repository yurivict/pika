# Copyright (c) 2019-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Function tests
set(function_tests
    # allocator_test
    bind_const_test
    bind_cv_test
    bind_dm2_test
    bind_dm3_test
    bind_dm_test
    bind_rv_sp_test
    bind_rvalue_test
    bind_stateful_test
    bind_test
    contains_test
    function_args
    function_arith
    function_bind_test
    function_object_size
    function_ref_wrapper
    function_target
    function_test
    nothrow_swap
    stateless_test
)

foreach(test ${function_tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  pika_add_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources}
    NOLIBS
    DEPENDENCIES pika
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Unit/Modules/Functional"
  )

  pika_add_unit_test("modules.functional" ${test})

endforeach()

if(PIKA_WITH_COMPILE_ONLY_TESTS)
  set(compile_tests sum_avg)

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    pika_add_unit_compile_test(
      "modules.functional" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Unit/Modules/Functional/CompileOnly"
    )
  endforeach()
endif()
