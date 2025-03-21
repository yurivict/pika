//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file wait_any.hpp

#pragma once

#if defined(DOXYGEN)
namespace pika {
    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param first    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_any should wait.
    /// \param last     [in] The iterator pointing to the last element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_any should wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_any(InputIter first, InputIter last);

    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param futures  [in] A vector holding an arbitrary amount of \a future or
    ///                 \a shared_future objects for which \a wait_any should
    ///                 wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename R>
    void wait_any(std::vector<future<R>>& futures);

    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param futures  [in] Amn array holding an arbitrary amount of \a future or
    ///                 \a shared_future objects for which \a wait_any should
    ///                 wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename R, std::size_t N>
    void wait_any(std::array<future<R>, N>& futures);

    /// The function \a wait_any is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param futures  [in] An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_any should wait.
    ///
    /// \note The function \a wait_any returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any
    ///       returns.
    ///
    /// \note           The function wait_any will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_nothrow
    ///                 instead.
    ///
    template <typename... T>
    void wait_any(T&&... futures);

    /// The function \a wait_any_n is a non-deterministic choice operator. It
    /// OR-composes all future objects given and returns after one future of
    /// that list finishes execution.
    ///
    /// \param first    [in] The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_any_n should wait.
    /// \param count    [in] The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \note The function \a wait_any_n returns after at least one future has
    ///       become ready. All input futures are still valid after \a wait_any_n
    ///       returns.
    ///
    /// \note           The function wait_any_n will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_any_n_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_any_n(InputIter first, std::size_t count);
}    // namespace pika

#else    // DOXYGEN

# include <pika/config.hpp>
# include <pika/async_combinators/wait_some.hpp>
# include <pika/futures/future.hpp>
# include <pika/iterator_support/traits/is_iterator.hpp>
# include <pika/preprocessor/strip_parens.hpp>

# include <array>
# include <cstddef>
# include <tuple>
# include <utility>
# include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace pika {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Future>
    void wait_any_nothrow(std::vector<Future> const& futures)
    {
        pika::wait_some_nothrow(1, futures);
    }

    template <typename Future>
    void wait_any(std::vector<Future> const& futures)
    {
        pika::wait_some(1, futures);
    }

    template <typename Future>
    void wait_any_nothrow(std::vector<Future>& lazy_values)
    {
        pika::wait_any_nothrow(const_cast<std::vector<Future> const&>(lazy_values));
    }

    template <typename Future>
    void wait_any(std::vector<Future>& lazy_values)
    {
        pika::wait_any(const_cast<std::vector<Future> const&>(lazy_values));
    }

    template <typename Future>
    void wait_any_nothrow(std::vector<Future>&& lazy_values)
    {
        pika::wait_any_nothrow(const_cast<std::vector<Future> const&>(lazy_values));
    }

    template <typename Future>
    void wait_any(std::vector<Future>&& lazy_values)
    {
        pika::wait_any(const_cast<std::vector<Future> const&>(lazy_values));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Future, std::size_t N>
    void wait_any_nothrow(std::array<Future, N> const& futures)
    {
        pika::wait_some_nothrow(1, futures);
    }

    template <typename Future, std::size_t N>
    void wait_any(std::array<Future, N> const& futures)
    {
        pika::wait_some(1, futures);
    }

    template <typename Future, std::size_t N>
    void wait_any_nothrow(std::array<Future, N>& lazy_values)
    {
        pika::wait_any_nothrow(const_cast<std::array<Future, N> const&>(lazy_values));
    }

    template <typename Future, std::size_t N>
    void wait_any(std::array<Future, N>& lazy_values)
    {
        pika::wait_any(const_cast<std::array<Future, N> const&>(lazy_values));
    }

    template <typename Future, std::size_t N>
    void wait_any_nothrow(std::array<Future, N>&& lazy_values)
    {
        pika::wait_any_nothrow(const_cast<std::array<Future, N> const&>(lazy_values));
    }

    template <typename Future, std::size_t N>
    void wait_any(std::array<Future, N>&& lazy_values)
    {
        pika::wait_any(const_cast<std::array<Future, N> const&>(lazy_values));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_any_nothrow(Iterator begin, Iterator end)
    {
        pika::wait_some_nothrow(1, begin, end);
    }

    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_any(Iterator begin, Iterator end)
    {
        pika::wait_some(1, begin, end);
    }

    inline void wait_any_nothrow()
    {
        pika::wait_some_nothrow(1);
    }

    inline void wait_any()
    {
        pika::wait_some(1);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_any_n_nothrow(Iterator begin, std::size_t count)
    {
        pika::wait_some_n_nothrow(1, begin, count);
    }

    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_any_n(Iterator begin, std::size_t count)
    {
        pika::wait_some_n(1, begin, count);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename... Ts>
    void wait_any_nothrow(Ts&&... ts)
    {
        pika::wait_some_nothrow(1, PIKA_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    void wait_any(Ts&&... ts)
    {
        pika::wait_some(1, PIKA_FORWARD(Ts, ts)...);
    }
}    // namespace pika
#endif    // DOXYGEN
