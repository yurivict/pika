# Copyright (c) 2020-2021 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    annotating_executor
    annotation_property
    created_executor
    fork_join_executor
    limiting_executor
    parallel_executor
    parallel_fork_executor
    parallel_policy_executor
    scheduler_executor
    sequenced_executor
    shared_parallel_executor
    standalone_thread_pool_executor
    std_thread_scheduler
    thread_pool_scheduler
)

if(PIKA_WITH_CXX17_STD_EXECUTION_POLICIES)
  set(tests ${tests} std_execution_policies)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  set(${test}_PARAMETERS THREADS 4)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Executors")

  # add example executable
  pika_add_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  pika_add_unit_test("modules.executors" ${test} ${${test}_PARAMETERS})

endforeach()
