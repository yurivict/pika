//  Copyright (c) 2008 Peter Dimov
//  Copyright (c) 2017 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config/compiler_specific.hpp>

#if defined(DOXYGEN)

/// Generates assembly that serves as a fence to the compiler CPU to disable
/// optimization. Usually implemented in the form of a memory barrier.
# define PIKA_COMPILER_FENCE
/// Generates assembly the executes a "pause" instruction. Useful in spinning
/// loops.
# define PIKA_SMT_PAUSE

#else
# if defined(__INTEL_COMPILER)

#  include <immintrin.h>
#  define PIKA_SMT_PAUSE _mm_pause()
#  define PIKA_COMPILER_FENCE __memory_barrier()

# elif defined(_MSC_VER) && _MSC_VER >= 1310

extern "C" void _ReadWriteBarrier();
#  pragma intrinsic(_ReadWriteBarrier)

#  define PIKA_COMPILER_FENCE _ReadWriteBarrier()

extern "C" void _mm_pause();
#  define PIKA_SMT_PAUSE _mm_pause()

# elif defined(__GNUC__)

#  define PIKA_COMPILER_FENCE __asm__ __volatile__("" : : : "memory")

#  if defined(__i386__) || defined(__x86_64__)
#   define PIKA_SMT_PAUSE __asm__ __volatile__("rep; nop" : : : "memory")
#  elif defined(__ppc__) || defined(__ppc64__)
// According to: https://stackoverflow.com/questions/5425506/equivalent-of-x86-pause-instruction-for-ppc
#   ifdef __APPLE__
#    define PIKA_SMT_PAUSE __asm__ volatile("or r27,r27,r27")
#   else
#    define PIKA_SMT_PAUSE __asm__ __volatile__("or 27,27,27")
#   endif
#  elif defined(__arm__)
#   define PIKA_SMT_PAUSE __asm__ __volatile__("yield")
#  else
#   define PIKA_SMT_PAUSE PIKA_COMPILER_FENCE
#  endif

# else

#  define PIKA_COMPILER_FENCE

# endif

# if !defined(PIKA_SMT_PAUSE)
#  define PIKA_SMT_PAUSE
# endif

#endif
