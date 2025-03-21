//  Copyright (c) 2006, Giovanni P. Deretta
//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  This code may be used under either of the following two licences:
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE. OF SUCH DAMAGE.
//
//  Or:
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>

#include <pika/assert.hpp>
#include <pika/coroutines/coroutine_fwd.hpp>
#include <pika/coroutines/detail/coroutine_accessor.hpp>
#include <pika/coroutines/detail/coroutine_impl.hpp>
#include <pika/coroutines/detail/coroutine_self.hpp>
#include <pika/coroutines/thread_enums.hpp>
#include <pika/coroutines/thread_id_type.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace pika::threads::coroutines::detail {
    /////////////////////////////////////////////////////////////////////////////
    class coroutine
    {
    public:
        friend struct coroutine_accessor;

        using impl_type = coroutine_impl;
        using thread_id_type = impl_type::thread_id_type;

        using result_type = impl_type::result_type;
        using arg_type = impl_type::arg_type;

        using functor_type = util::detail::unique_function<result_type(arg_type)>;

        coroutine(
            functor_type&& f, thread_id_type id, std::ptrdiff_t stack_size = default_stack_size)
          : impl_(PIKA_MOVE(f), id, stack_size)
        {
            PIKA_ASSERT(impl_.is_ready());
        }

        coroutine(coroutine const& src) = delete;
        coroutine& operator=(coroutine const& src) = delete;
        coroutine(coroutine&& src) = delete;
        coroutine& operator=(coroutine&& src) = delete;

        thread_id_type get_thread_id() const
        {
            return impl_.get_thread_id();
        }

#if defined(PIKA_HAVE_THREAD_PHASE_INFORMATION)
        std::size_t get_thread_phase() const
        {
            return impl_.get_thread_phase();
        }
#endif

        std::size_t get_thread_data() const
        {
            return impl_.get_thread_data();
        }

        std::size_t set_thread_data(std::size_t data)
        {
            return impl_.set_thread_data(data);
        }

        void init()
        {
            impl_.init();
        }

        void rebind(functor_type&& f, thread_id_type id)
        {
            impl_.rebind(PIKA_MOVE(f), id);
        }

        PIKA_FORCEINLINE result_type operator()(arg_type arg = arg_type())
        {
            PIKA_ASSERT(impl_.is_ready());

            impl_.bind_args(&arg);

            impl_.invoke();

            return impl_.result();
        }

        bool is_ready() const
        {
            return impl_.is_ready();
        }

        std::ptrdiff_t get_available_stack_space()
        {
#if defined(PIKA_HAVE_THREADS_GET_STACK_POINTER)
            return impl_.get_available_stack_space();
#else
            return (std::numeric_limits<std::ptrdiff_t>::max)();
#endif
        }

        impl_type* impl()
        {
            return &impl_;
        }

    private:
        impl_type impl_;
    };
}    // namespace pika::threads::coroutines::detail
