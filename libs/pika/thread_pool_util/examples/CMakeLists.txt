# Copyright (c) 2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(PIKA_WITH_EXAMPLES)
  pika_add_pseudo_target(examples.modules.thread_pool_util)
  pika_add_pseudo_dependencies(
    examples.modules examples.modules.thread_pool_util
  )
  if(PIKA_WITH_TESTS AND PIKA_WITH_TESTS_EXAMPLES)
    pika_add_pseudo_target(tests.examples.modules.thread_pool_util)
    pika_add_pseudo_dependencies(
      tests.examples.modules tests.examples.modules.thread_pool_util
    )
  endif()
endif()
