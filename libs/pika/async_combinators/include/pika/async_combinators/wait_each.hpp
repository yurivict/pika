//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//  Copyright (c) 2016 Lukas Troska
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(DOXYGEN)
namespace pika {
    /// The function \a wait_each is an operator allowing to join on the results
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready. \a wait_each returns
    /// after all futures have been become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param futures  A vector holding an arbitrary amount of \a future or
    ///                 \a shared_future objects for which \a wait_each should
    ///                 wait.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename Future>
    void wait_each(F&& f, std::vector<Future>&& futures);

    /// The function \a wait_each is an operator allowing to join on the results
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready. \a wait_each returns
    /// after all futures have been become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_each should wait.
    /// \param end      The iterator pointing to the last element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_each should wait.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename Iterator>
    void wait_each(F&& f, Iterator begin, Iterator end);

    /// The function \a wait_each is an operator allowing to join on the results
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready. \a wait_each returns
    /// after all futures have been become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param futures  An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_each should wait.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename... T>
    void wait_each(F&& f, T&&... futures);

    /// The function \a wait_each is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    /// Additionally, the supplied function is called for each of the passed
    /// futures as soon as the future has become ready.
    ///
    /// \param f        The function which will be called for each of the
    ///                 input futures once the future has become ready.
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_each_n should wait.
    /// \param count    The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \note This function consumes the futures as they are passed on to the
    ///       supplied function. The callback should take one or two parameters,
    ///       namely either a \a future to be processed or a type that
    ///       \a std::size_t is implicitly convertible to as the
    ///       first parameter and the \a future as the second
    ///       parameter. The first parameter will correspond to the
    ///       index of the current \a future in the collection.
    ///
    template <typename F, typename Iterator>
    void wait_each_n(F&& f, Iterator begin, std::size_t count);
}    // namespace pika

#else    // DOXYGEN

# include <pika/config.hpp>
# include <pika/async_combinators/detail/throw_if_exceptional.hpp>
# include <pika/async_combinators/when_each.hpp>
# include <pika/futures/traits/is_future.hpp>
# include <pika/iterator_support/traits/is_iterator.hpp>
# include <pika/type_support/pack.hpp>

# include <cstddef>
# include <type_traits>
# include <utility>
# include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace pika {
    template <typename F, typename Future>
    void wait_each_nothrow(F&& f, std::vector<Future>& values)
    {
        pika::when_each(PIKA_FORWARD(F, f), values).wait();
    }

    template <typename F, typename Future>
    void wait_each(F&& f, std::vector<Future>& values)
    {
        auto result = pika::when_each(PIKA_FORWARD(F, f), values);
        result.wait();
        pika::detail::throw_if_exceptional(PIKA_MOVE(result));
    }

    template <typename F, typename Future>
    void wait_each_nothrow(F&& f, std::vector<Future>&& values)
    {
        pika::when_each(PIKA_FORWARD(F, f), PIKA_MOVE(values)).wait();
    }

    template <typename F, typename Future>
    void wait_each(F&& f, std::vector<Future>&& values)
    {
        auto result = pika::when_each(PIKA_FORWARD(F, f), PIKA_MOVE(values));
        result.wait();
        pika::detail::throw_if_exceptional(PIKA_MOVE(result));
    }

    template <typename F, typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_each_nothrow(F&& f, Iterator begin, Iterator end)
    {
        pika::when_each(PIKA_FORWARD(F, f), begin, end).wait();
    }

    template <typename F, typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_each(F&& f, Iterator begin, Iterator end)
    {
        auto result = pika::when_each(PIKA_FORWARD(F, f), begin, end);
        result.wait();
        pika::detail::throw_if_exceptional(PIKA_MOVE(result));
    }

    template <typename F, typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_each_n_nothrow(F&& f, Iterator begin, std::size_t count)
    {
        pika::when_each_n(PIKA_FORWARD(F, f), begin, count).wait();
    }

    template <typename F, typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_each_n(F&& f, Iterator begin, std::size_t count)
    {
        auto result = pika::when_each_n(PIKA_FORWARD(F, f), begin, count);
        result.wait();
        pika::detail::throw_if_exceptional(PIKA_MOVE(result));
    }

    template <typename F>
    void wait_each_nothrow(F&& f)
    {
        pika::when_each(PIKA_FORWARD(F, f)).wait();
    }

    template <typename F>
    void wait_each(F&& f)
    {
        auto result = pika::when_each(PIKA_FORWARD(F, f));
        result.wait();
        pika::detail::throw_if_exceptional(PIKA_MOVE(result));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename F, typename... Ts,
        typename Enable = std::enable_if_t<!traits::is_future_v<std::decay_t<F>> &&
            util::detail::all_of_v<traits::is_future<Ts>...>>>
    void wait_each_nothrow(F&& f, Ts&&... ts)
    {
        pika::when_each(PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...).wait();
    }

    template <typename F, typename... Ts,
        typename Enable = std::enable_if_t<!traits::is_future_v<std::decay_t<F>> &&
            util::detail::all_of_v<traits::is_future<Ts>...>>>
    void wait_each(F&& f, Ts&&... ts)
    {
        auto result = pika::when_each(PIKA_FORWARD(F, f), PIKA_FORWARD(Ts, ts)...);
        result.wait();
        pika::detail::throw_if_exceptional(PIKA_MOVE(result));
    }
}    // namespace pika
#endif    // DOXYGEN
