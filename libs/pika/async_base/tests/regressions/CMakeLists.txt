# Copyright (c) 2022 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(PIKA_WITH_COMPILE_ONLY_TESTS)
  set(compile_tests prefer_scheduling_properties)

  foreach(compile_test ${compile_tests})
    set(sources ${compile_test}.cpp)

    source_group("Source Files" FILES ${sources})

    pika_add_regression_compile_test(
      "modules.async_base" ${compile_test}
      SOURCES ${sources} ${${compile_test}_FLAGS}
      FOLDER "Tests/Regressions/Modules/AsyncBase/CompileOnly"
    )
  endforeach()
endif()
