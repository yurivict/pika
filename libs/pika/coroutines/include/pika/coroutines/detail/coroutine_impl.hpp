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

#if defined(PIKA_MSVC_WARNING_PRAGMA)
# pragma warning(push)
# pragma warning(disable : 4355)    //this used in base member initializer
#endif

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/coroutines/coroutine_fwd.hpp>
#include <pika/coroutines/detail/context_base.hpp>
#include <pika/coroutines/detail/coroutine_accessor.hpp>
#include <pika/coroutines/thread_enums.hpp>
#include <pika/coroutines/thread_id_type.hpp>
#include <pika/functional/unique_function.hpp>

#include <cstddef>
#include <utility>

namespace pika::threads::coroutines::detail {

    ///////////////////////////////////////////////////////////////////////////
    // This type augments the context_base type with the type of the stored
    // functor.
    class coroutine_impl : public context_base<coroutine_impl>
    {
    public:
        PIKA_NON_COPYABLE(coroutine_impl);

    public:
        using super_type = context_base;
        using thread_id_type = context_base::thread_id_type;

        using result_type = std::pair<threads::detail::thread_schedule_state, thread_id_type>;
        using arg_type = threads::detail::thread_restart_state;

        using functor_type = util::detail::unique_function<result_type(arg_type)>;

        coroutine_impl(functor_type&& f, thread_id_type id, std::ptrdiff_t stack_size)
          : context_base(stack_size, id)
          , m_result(
                threads::detail::thread_schedule_state::unknown, threads::detail::invalid_thread_id)
          , m_arg(nullptr)
          , m_fun(PIKA_MOVE(f))
        {
        }

#if defined(PIKA_DEBUG)
        PIKA_EXPORT ~coroutine_impl();
#endif

        // execute the coroutine using normal context switching
        PIKA_EXPORT void operator()() noexcept;

    public:
        void bind_result(result_type res)
        {
            PIKA_ASSERT(m_result.first != threads::detail::thread_schedule_state::terminated);
            m_result = res;
        }

        result_type result() const
        {
            return m_result;
        }
        arg_type* args() noexcept
        {
            PIKA_ASSERT(m_arg);
            return m_arg;
        };

        void bind_args(arg_type* arg) noexcept
        {
            m_arg = arg;
        }

#if defined(PIKA_HAVE_THREAD_PHASE_INFORMATION)
        std::size_t get_thread_phase() const
        {
            return this->phase();
        }
#endif

        void init()
        {
            this->super_type::init();
        }

        void reset()
        {
            // First reset the function and arguments
            m_arg = nullptr;
            m_fun.reset();

            // Then reset the id and stack as they may be used by the
            // destructors of the thread function above
            this->super_type::reset();
            this->reset_stack();
        }

        void rebind(functor_type&& f, thread_id_type id)
        {
            PIKA_ASSERT(m_result.first == threads::detail::thread_schedule_state::unknown ||
                m_result.first == threads::detail::thread_schedule_state::terminated);
            this->rebind_stack();    // count how often a coroutines object was reused
            m_result = result_type(threads::detail::thread_schedule_state::unknown,
                threads::detail::invalid_thread_id);
            m_arg = nullptr;
            m_fun = PIKA_MOVE(f);
            this->super_type::rebind_base(id);
        }

    private:
        result_type m_result;
        arg_type* m_arg;
        functor_type m_fun;
    };
}    // namespace pika::threads::coroutines::detail

#if defined(PIKA_MSVC_WARNING_PRAGMA)
# pragma warning(pop)
#endif
