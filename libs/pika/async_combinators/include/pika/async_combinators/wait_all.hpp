//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file wait_all.hpp

#pragma once

#if defined(DOXYGEN)
namespace pika {
    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param first    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all should wait.
    /// \param last     The iterator pointing to the last element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_all(InputIter first, InputIter last);

    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    template <typename R>
    void wait_all(std::vector<future<R>>&& futures);

    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param futures  A vector or array holding an arbitrary amount of
    ///                 \a future or \a shared_future objects for which
    ///                 \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    template <typename R, std::size_t N>
    void wait_all(std::array<future<R>, N>&& futures);

    /// The function \a wait_all is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param futures  An arbitrary number of \a future or \a shared_future
    ///                 objects, possibly holding different types for which
    ///                 \a wait_all should wait.
    ///
    /// \note The function \a wait_all returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all
    ///       returns.
    ///
    /// \note           The function wait_all will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_nothrow
    ///                 instead.
    ///
    template <typename... T>
    void wait_all(T&&... futures);

    /// The function \a wait_all_n is an operator allowing to join on the result
    /// of all given futures. It AND-composes all future objects given and
    /// returns after they finished executing.
    ///
    /// \param begin    The iterator pointing to the first element of a
    ///                 sequence of \a future or \a shared_future objects for
    ///                 which \a wait_all_n should wait.
    /// \param count    The number of elements in the sequence starting at
    ///                 \a first.
    ///
    /// \return         The function \a wait_all_n will return an iterator
    ///                 referring to the first element in the input sequence
    ///                 after the last processed element.
    ///
    /// \note The function \a wait_all_n returns after all futures have become
    ///       ready. All input futures are still valid after \a wait_all_n
    ///       returns.
    ///
    /// \note           The function wait_all_n will rethrow any exceptions
    ///                 captured by the futures while becoming ready. If this
    ///                 behavior is undesirable, use \a wait_all_n_nothrow
    ///                 instead.
    ///
    template <typename InputIter>
    void wait_all_n(InputIter begin, std::size_t count);
}    // namespace pika

#else    // DOXYGEN

# include <pika/config.hpp>
# include <pika/async_combinators/detail/throw_if_exceptional.hpp>
# include <pika/futures/detail/future_data.hpp>
# include <pika/futures/traits/acquire_shared_state.hpp>
# include <pika/futures/traits/detail/future_traits.hpp>
# include <pika/futures/traits/future_access.hpp>
# include <pika/futures/traits/is_future.hpp>
# include <pika/iterator_support/range.hpp>
# include <pika/iterator_support/traits/is_iterator.hpp>
# include <pika/memory/intrusive_ptr.hpp>
# include <pika/type_support/decay.hpp>
# include <pika/type_support/unwrap_reference.hpp>

# include <algorithm>
# include <array>
# include <cstddef>
# include <functional>
# include <iterator>
# include <tuple>
# include <type_traits>
# include <utility>
# include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace pika {

    // forward declare wait_all()
    template <typename Future>
    void wait_all(std::vector<Future>&& values);

    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        template <typename Future, typename Enable = void>
        struct is_future_or_shared_state : traits::is_future<Future>
        {
        };

        template <typename R>
        struct is_future_or_shared_state<
            pika::intrusive_ptr<pika::lcos::detail::future_data_base<R>>> : std::true_type
        {
        };

        template <typename R>
        struct is_future_or_shared_state<std::reference_wrapper<R>> : is_future_or_shared_state<R>
        {
        };

        template <typename R>
        inline constexpr bool is_future_or_shared_state_v = is_future_or_shared_state<R>::value;

        ///////////////////////////////////////////////////////////////////////
        template <typename Range, typename Enable = void>
        struct is_future_or_shared_state_range : std::false_type
        {
        };

        template <typename T>
        struct is_future_or_shared_state_range<std::vector<T>> : is_future_or_shared_state<T>
        {
        };

        template <typename T, std::size_t N>
        struct is_future_or_shared_state_range<std::array<T, N>> : is_future_or_shared_state<T>
        {
        };

        template <typename R>
        inline constexpr bool is_future_or_shared_state_range_v =
            is_future_or_shared_state_range<R>::value;

        ///////////////////////////////////////////////////////////////////////
        template <typename Future, typename Enable = void>
        struct future_or_shared_state_result;

        template <typename Future>
        struct future_or_shared_state_result<Future,
            std::enable_if_t<pika::traits::is_future_v<Future>>>
          : pika::traits::future_traits<Future>
        {
        };

        template <typename R>
        struct future_or_shared_state_result<
            pika::intrusive_ptr<pika::lcos::detail::future_data_base<R>>>
        {
            using type = R;
        };

        template <typename R>
        using future_or_shared_state_result_t = typename future_or_shared_state_result<R>::type;

        ///////////////////////////////////////////////////////////////////////
        template <typename Tuple>
        struct wait_all_frame    //-V690
          : pika::lcos::detail::future_data<void>
        {
        private:
            using base_type = pika::lcos::detail::future_data<void>;
            using init_no_addref = typename base_type::init_no_addref;

            wait_all_frame(wait_all_frame const&) = delete;
            wait_all_frame(wait_all_frame&&) = delete;

            wait_all_frame& operator=(wait_all_frame const&) = delete;
            wait_all_frame& operator=(wait_all_frame&&) = delete;

            template <std::size_t I>
            struct is_end : std::integral_constant<bool, std::tuple_size<Tuple>::value == I>
            {
            };

            template <std::size_t I>
            static constexpr bool is_end_v = is_end<I>::value;

        public:
            wait_all_frame(Tuple const& t)
              : base_type(init_no_addref{})
              , t_(t)
            {
            }

        protected:
            // Current element is a range (vector or array) of futures
            template <std::size_t I, typename Iter>
            void await_range(Iter&& next, Iter&& end)
            {
                pika::intrusive_ptr<wait_all_frame> this_(this);
                for (/**/; next != end; ++next)
                {
                    auto next_future_data = pika::traits::detail::get_shared_state(*next);

                    if (next_future_data && !next_future_data->is_ready())
                    {
                        next_future_data->execute_deferred();

                        // execute_deferred might have made the future ready
                        if (!next_future_data->is_ready())
                        {
                            // Attach a continuation to this future which will
                            // re-evaluate it and continue to the next element
                            // in the sequence (if any).
                            next_future_data->set_on_completed(
                                [this_ = PIKA_MOVE(this_), next = PIKA_FORWARD(Iter, next),
                                    end = PIKA_FORWARD(Iter, end)]() mutable -> void {
                                    this_->template await_range<I>(PIKA_MOVE(next), PIKA_MOVE(end));
                                });

                            // explicitly destruct iterators as those might
                            // become dangling after we make ourselves ready
                            next = std::decay_t<Iter>{};
                            end = std::decay_t<Iter>{};
                            return;
                        }
                    }
                }

                // explicitly destruct iterators as those might become dangling
                // after we make ourselves ready
                next = std::decay_t<Iter>{};
                end = std::decay_t<Iter>{};

                // All elements of the sequence are ready now, proceed to the
                // next argument.
                do_await<I + 1>();
            }

            template <std::size_t I>
            PIKA_FORCEINLINE void await_range()
            {
                await_range<I>(pika::util::begin(pika::detail::unwrap_reference(std::get<I>(t_))),
                    pika::util::end(pika::detail::unwrap_reference(std::get<I>(t_))));
            }

            // Current element is a simple future
            template <std::size_t I>
            PIKA_FORCEINLINE void await_future()
            {
                pika::intrusive_ptr<wait_all_frame> this_(this);
                auto next_future_data = pika::traits::detail::get_shared_state(std::get<I>(t_));

                if (next_future_data && !next_future_data->is_ready())
                {
                    next_future_data->execute_deferred();

                    // execute_deferred might have made the future ready
                    if (!next_future_data->is_ready())
                    {
                        // Attach a continuation to this future which will
                        // re-evaluate it and continue to the next argument
                        // (if any).
                        next_future_data->set_on_completed([this_ = PIKA_MOVE(this_)]() -> void {
                            this_->template await_future<I>();
                        });

                        return;
                    }
                }

                do_await<I + 1>();
            }

            template <std::size_t I>
            PIKA_FORCEINLINE void do_await()
            {
                // Check if end of the tuple is reached
                if constexpr (is_end_v<I>)
                {
                    // simply make ourself ready
                    this->set_value(util::detail::unused);
                }
                else
                {
                    using future_type =
                        pika::detail::decay_unwrap_t<typename std::tuple_element<I, Tuple>::type>;

                    if constexpr (is_future_or_shared_state_v<future_type>)
                    {
                        await_future<I>();
                    }
                    else
                    {
                        static_assert(is_future_or_shared_state_range_v<future_type>,
                            "element must be future or range of futures");
                        await_range<I>();
                    }
                }
            }

        public:
            void wait_all()
            {
                do_await<0>();

                // If there are still futures which are not ready, suspend
                // and wait.
                if (!this->is_ready())
                {
                    this->wait();
                }
            }

        private:
            Tuple const& t_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Future>
    void wait_all_nothrow(std::vector<Future> const& values)
    {
        if (!values.empty())
        {
            using result_type = std::tuple<std::vector<Future> const&>;
            using frame_type = pika::detail::wait_all_frame<result_type>;

            result_type data(values);

            // frame is initialized with initial reference count
            pika::intrusive_ptr<frame_type> frame(new frame_type(data), false);
            frame->wait_all();
        }
    }

    template <typename Future>
    void wait_all(std::vector<Future> const& values)
    {
        pika::wait_all_nothrow(values);
        pika::detail::throw_if_exceptional(values);
    }

    template <typename Future>
    PIKA_FORCEINLINE void wait_all_nothrow(std::vector<Future>& values)
    {
        pika::wait_all_nothrow(const_cast<std::vector<Future> const&>(values));
    }

    template <typename Future>
    PIKA_FORCEINLINE void wait_all(std::vector<Future>& values)
    {
        pika::wait_all_nothrow(const_cast<std::vector<Future> const&>(values));
        pika::detail::throw_if_exceptional(values);
    }

    template <typename Future>
# if !defined(PIKA_INTEL_VERSION)
    PIKA_FORCEINLINE
# endif
        void
        wait_all_nothrow(std::vector<Future>&& values)
    {
        pika::wait_all_nothrow(const_cast<std::vector<Future> const&>(values));
    }

    template <typename Future>
# if !defined(PIKA_INTEL_VERSION)
    PIKA_FORCEINLINE
# endif
        void
        wait_all(std::vector<Future>&& values)
    {
        pika::wait_all_nothrow(const_cast<std::vector<Future> const&>(values));
        pika::detail::throw_if_exceptional(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Future, std::size_t N>
    void wait_all_nothrow(std::array<Future, N> const& values)
    {
        using result_type = std::tuple<std::array<Future, N> const&>;
        using frame_type = pika::detail::wait_all_frame<result_type>;

        result_type data(values);

        // frame is initialized with initial reference count
        pika::intrusive_ptr<frame_type> frame(new frame_type(data), false);
        frame->wait_all();
    }

    template <typename Future, std::size_t N>
    void wait_all(std::array<Future, N> const& values)
    {
        pika::wait_all_nothrow(values);
        pika::detail::throw_if_exceptional(values);
    }

    template <typename Future, std::size_t N>
    PIKA_FORCEINLINE void wait_all_nothrow(std::array<Future, N>& values)
    {
        pika::wait_all_nothrow(const_cast<std::array<Future, N> const&>(values));
    }

    template <typename Future, std::size_t N>
    PIKA_FORCEINLINE void wait_all(std::array<Future, N>& values)
    {
        pika::wait_all_nothrow(const_cast<std::array<Future, N> const&>(values));
        pika::detail::throw_if_exceptional(values);
    }

    template <typename Future, std::size_t N>
    PIKA_FORCEINLINE void wait_all_nothrow(std::array<Future, N>&& values)
    {
        pika::wait_all_nothrow(const_cast<std::array<Future, N> const&>(values));
    }

    template <typename Future, std::size_t N>
    PIKA_FORCEINLINE void wait_all(std::array<Future, N>&& values)
    {
        pika::wait_all_nothrow(const_cast<std::array<Future, N> const&>(values));
        pika::detail::throw_if_exceptional(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_all_nothrow(Iterator begin, Iterator end)
    {
        if (begin != end)
        {
            auto values = traits::acquire_shared_state<Iterator>()(begin, end);
            pika::wait_all_nothrow(values);
        }
    }

    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_all(Iterator begin, Iterator end)
    {
        if (begin != end)
        {
            auto values = traits::acquire_shared_state<Iterator>()(begin, end);
            pika::wait_all_nothrow(values);
            pika::detail::throw_if_exceptional(values);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_all_n_nothrow(Iterator begin, std::size_t count)
    {
        if (count != 0)
        {
            auto values = traits::acquire_shared_state<Iterator>()(begin, count);
            pika::wait_all_nothrow(values);
        }
    }

    template <typename Iterator,
        typename Enable = std::enable_if_t<pika::traits::is_iterator_v<Iterator>>>
    void wait_all_n(Iterator begin, std::size_t count)
    {
        if (count != 0)
        {
            auto values = traits::acquire_shared_state<Iterator>()(begin, count);
            pika::wait_all_nothrow(values);
            pika::detail::throw_if_exceptional(values);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    constexpr inline void wait_all_nothrow() noexcept {}
    constexpr inline void wait_all() noexcept {}

    ///////////////////////////////////////////////////////////////////////////
    template <typename... Ts>
    void wait_all_nothrow(Ts&&... ts)
    {
        if constexpr (sizeof...(Ts) != 0)
        {
            using result_type = std::tuple<traits::detail::shared_state_ptr_for_t<Ts>...>;
            using frame_type = detail::wait_all_frame<result_type>;

            result_type values = result_type(pika::traits::detail::get_shared_state(ts)...);

            // frame is initialized with initial reference count
            pika::intrusive_ptr<frame_type> frame(new frame_type(values), false);
            frame->wait_all();
        }
    }

    template <typename... Ts>
    void wait_all(Ts&&... ts)
    {
        pika::wait_all_nothrow(ts...);
        pika::detail::throw_if_exceptional(PIKA_FORWARD(Ts, ts)...);
    }
}    // namespace pika
#endif    // DOXYGEN
