//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config/defines.hpp>
#include <pika/preprocessor/identity.hpp>

#if defined(PIKA_HAVE_CXX20_LAMBDA_CAPTURE)
# define PIKA_CXX20_CAPTURE_THIS(...) PIKA_PP_IDENTITY(__VA_ARGS__, this)
#else
# define PIKA_CXX20_CAPTURE_THIS(...) __VA_ARGS__
#endif
