////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <pika/config.hpp>

#if defined(PIKA_MSVC)
# include <pika/timing/detail/timestamp/msvc.hpp>
#elif defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) ||        \
    defined(_M_X64)
# if defined(PIKA_HAVE_RDTSC) || defined(PIKA_HAVE_RDTSCP)
#  include <pika/timing/detail/timestamp/linux_x86_64.hpp>
# else
#  include <pika/timing/detail/timestamp/linux_generic.hpp>
# endif
#elif defined(i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) ||              \
    defined(__i686__) || defined(__i386) || defined(_M_IX86) || defined(__X86__) ||                \
    defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__)
# if defined(PIKA_HAVE_RDTSC) || defined(PIKA_HAVE_RDTSCP)
#  include <pika/timing/detail/timestamp/linux_x86_32.hpp>
# else
#  include <pika/timing/detail/timestamp/linux_generic.hpp>
# endif
#elif (defined(__ANDROID__) && defined(ANDROID))
# include <pika/timing/detail/timestamp/linux_generic.hpp>
#elif defined(__arm__) || defined(__arm64__) || defined(__aarch64__)
# include <pika/timing/detail/timestamp/linux_generic.hpp>
#elif defined(__ppc__) || defined(__ppc) || defined(__powerpc__)
# include <pika/timing/detail/timestamp/linux_generic.hpp>
#elif defined(__s390x__)
# include <pika/timing/detail/timestamp/linux_generic.hpp>
#elif defined(__bgq__)
# include <pika/timing/detail/timestamp/bgq.hpp>
#else
# error Unsupported platform.
#endif
