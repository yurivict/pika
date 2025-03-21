//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/datastructures/traits/is_tuple_like.hpp>
#include <pika/functional/detail/invoke.hpp>
#include <pika/functional/invoke_fused.hpp>
#include <pika/futures/traits/future_traits.hpp>
#include <pika/futures/traits/is_future.hpp>
#include <pika/pack_traversal/pack_traversal.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace pika::util::detail {
    /// Deduces to a true_type if the given future is instantiated with
    /// a non void type.
    template <typename T>
    using is_non_void_future = std::integral_constant<bool,
        traits::is_future<T>::value &&
            !std::is_void<typename traits::future_traits<T>::result_type>::value>;

    /// Deduces to a true_type if the given future is instantiated with void
    template <typename T>
    using is_void_future = std::integral_constant<bool,
        traits::is_future<T>::value &&
            std::is_void<typename traits::future_traits<T>::result_type>::value>;

    /// A mapper that maps futures to its representing type
    ///
    /// The mapper does unwrap futures nested inside futures until
    /// the particular given depth.
    ///
    /// - Depth >  1 -> Depth remaining
    /// - Depth == 1 -> One depth remaining
    /// - Depth == 0 -> Unlimited depths
    template <std::size_t Depth>
    struct future_unwrap_until_depth
    {
        /// This piece of code can't be refactored out using
        /// inheritance and `using Base::operator()` because this
        /// isn't taken into account when doing SFINAE.
        template <typename T,
            typename std::enable_if<is_void_future<std::decay_t<T>>::value>::type* = nullptr>
        auto operator()(T&& future) const -> decltype(spread_this())
        {
#if defined(PIKA_CUDA_VERSION)
            std::forward<T>(future).get();
#else
            PIKA_FORWARD(T, future).get();
#endif
            return spread_this();
        }

        template <typename T,
            typename std::enable_if<is_non_void_future<std::decay_t<T>>::value>::type* = nullptr>
        auto operator()(T&& future) const
            -> decltype(map_pack(future_unwrap_until_depth<Depth - 1>{},
#if defined(PIKA_CUDA_VERSION)
                std::forward<T>(future).get()))
#else
                PIKA_FORWARD(T, future).get()))
#endif
        {
            return map_pack(future_unwrap_until_depth<Depth - 1>{},
#if defined(PIKA_CUDA_VERSION)
                std::forward<T>(future).get());
#else
                PIKA_FORWARD(T, future).get());
#endif
        }
    };
    template <>
    struct future_unwrap_until_depth<1U>
    {
        /// This piece of code can't be refactored out using
        /// inheritance and `using Base::operator()` because this
        /// isn't taken into account when doing SFINAE.
        template <typename T,
            typename std::enable_if<is_void_future<std::decay_t<T>>::value>::type* = nullptr>
        auto operator()(T&& future) const -> decltype(spread_this())
        {
#if defined(PIKA_CUDA_VERSION)
            std::forward<T>(future).get();
#else
            PIKA_FORWARD(T, future).get();
#endif
            return spread_this();
        }

        template <typename T,
            typename std::enable_if<is_non_void_future<std::decay_t<T>>::value>::type* = nullptr>
        auto operator()(T&& future) const ->
            typename traits::future_traits<std::decay_t<T>>::result_type
        {
#if defined(PIKA_CUDA_VERSION)
            return std::forward<T>(future).get();
#else
            return PIKA_FORWARD(T, future).get();
#endif
        }
    };
    template <>
    struct future_unwrap_until_depth<0U>
    {
        /// This piece of code can't be refactored out using
        /// inheritance and `using Base::operator()` because this
        /// isn't taken into account when doing SFINAE.
        template <typename T,
            typename std::enable_if<is_void_future<std::decay_t<T>>::value>::type* = nullptr>
        auto operator()(T&& future) const -> decltype(spread_this())
        {
#if defined(PIKA_CUDA_VERSION)
            std::forward<T>(future).get();
#else
            PIKA_FORWARD(T, future).get();
#endif
            return spread_this();
        }

        template <typename T,
            typename std::enable_if<is_non_void_future<std::decay_t<T>>::value>::type* = nullptr>
        auto operator()(T&& future) const
            -> decltype(map_pack(std::declval<future_unwrap_until_depth const&>(),
#if defined(PIKA_CUDA_VERSION)
                std::forward<T>(future).get()))
#else
                PIKA_FORWARD(T, future).get()))
#endif
        {
#if defined(PIKA_CUDA_VERSION)
            return map_pack(*this, std::forward<T>(future).get());
#else
            return map_pack(*this, PIKA_FORWARD(T, future).get());
#endif
        }
    };

    /// Unwraps the futures contained in the given pack args
    /// until the depth Depth.
    /// This is the main entry function for immediate unwraps.
    template <std::size_t Depth, typename... Args>
    auto unwrap_depth_impl(Args&&... args)
        -> decltype(map_pack(future_unwrap_until_depth<Depth>{}, PIKA_FORWARD(Args, args)...))
    {
        return map_pack(future_unwrap_until_depth<Depth>{}, PIKA_FORWARD(Args, args)...);
    }

    /// We use a specialized class here because MSVC has issues with
    /// tag dispatching a function because it does semantical checks before
    /// matching the tag, which leads to false errors.
    template <bool IsFusedInvoke>
    struct invoke_wrapped_invocation_select
    {
        /// Invoke the callable with the tuple argument through invoke_fused
        template <typename C, typename T>
        static auto apply(C&& callable, T&& unwrapped)
            // There is no trait for the invoke_fused result
            -> decltype(invoke_fused(PIKA_FORWARD(C, callable), PIKA_FORWARD(T, unwrapped)))
        {
            return invoke_fused(PIKA_FORWARD(C, callable), PIKA_FORWARD(T, unwrapped));
        }
    };
    template <>
    struct invoke_wrapped_invocation_select<false /*IsFusedInvoke*/>
    {
        /// Invoke the callable with the plain argument through invoke,
        /// also when the result is a tuple like type, when we received
        /// a single argument.
        template <typename C, typename T>
        static auto apply(C&& callable, T&& unwrapped) -> std::invoke_result_t<C, T>
        {
            return PIKA_INVOKE(PIKA_FORWARD(C, callable), PIKA_FORWARD(T, unwrapped));
        }
    };

    /// Deduces to a true_type if the result of unwrap should
    /// be fused invoked which is the case when:
    /// - The callable was called with more than one argument
    /// - The result of the unwrap is a tuple like type
    template <bool HadMultipleArguments, typename T>
    using should_fuse_invoke = std::integral_constant<bool,
        (HadMultipleArguments && traits::detail::is_tuple_like_v<std::decay_t<T>>)>;

    /// Invokes the callable object with the result:
    /// - If the result is a tuple-like type `invoke_fused` is used
    /// - Otherwise `invoke` is used
    template <bool HadMultipleArguments, typename C, typename T>
    auto dispatch_wrapped_invocation_select(C&& callable, T&& unwrapped)
        -> decltype(invoke_wrapped_invocation_select<
            should_fuse_invoke<HadMultipleArguments, T>::value>::apply(PIKA_FORWARD(C, callable),
            PIKA_FORWARD(T, unwrapped)))
    {
        return invoke_wrapped_invocation_select<
            should_fuse_invoke<HadMultipleArguments, T>::value>::apply(PIKA_FORWARD(C, callable),
            PIKA_FORWARD(T, unwrapped));
    }

    /// Helper for routing non void result types to the corresponding
    /// callable object.
    template <std::size_t Depth, typename Result>
    struct invoke_wrapped_decorate_select
    {
        template <typename C, typename... Args>
        static auto apply(C&& callable, Args&&... args)
            -> decltype(dispatch_wrapped_invocation_select<(sizeof...(args) > 1)>(
                PIKA_FORWARD(C, callable), unwrap_depth_impl<Depth>(PIKA_FORWARD(Args, args)...)))
        {
            return dispatch_wrapped_invocation_select<(sizeof...(args) > 1)>(
                PIKA_FORWARD(C, callable), unwrap_depth_impl<Depth>(PIKA_FORWARD(Args, args)...));
        }
    };
    template <std::size_t Depth>
    struct invoke_wrapped_decorate_select<Depth, void>
    {
        template <typename C, typename... Args>
        static auto apply(C&& callable, Args&&... args) -> decltype(PIKA_FORWARD(C, callable)())
        {
            unwrap_depth_impl<Depth>(PIKA_FORWARD(Args, args)...);
            return PIKA_FORWARD(C, callable)();
        }
    };

    /// map_pack may return a tuple, a plain type or void choose the
    /// corresponding invocation function accordingly.
    template <std::size_t Depth, typename C, typename... Args>
    auto invoke_wrapped(C&& callable, Args&&... args)
        -> decltype(invoke_wrapped_decorate_select<Depth,
            decltype(unwrap_depth_impl<Depth>(PIKA_FORWARD(Args, args)...))>::apply(PIKA_FORWARD(C,
                                                                                        callable),
            PIKA_FORWARD(Args, args)...))
    {
        return invoke_wrapped_decorate_select<Depth,
            decltype(unwrap_depth_impl<Depth>(PIKA_FORWARD(Args, args)...))>::apply(PIKA_FORWARD(C,
                                                                                        callable),
            PIKA_FORWARD(Args, args)...);
    }

    /// Implements the callable object which is returned by n invocation
    /// to pika::unwrap and similar functions.
    template <typename T, std::size_t Depth>
    class functional_unwrap_impl
    {
        /// The wrapped callable object
        T wrapped_;

    public:
        explicit functional_unwrap_impl(T wrapped)
          : wrapped_(PIKA_MOVE(wrapped))
        {
        }

        template <typename... Args>
        auto operator()(Args&&... args)
            -> decltype(invoke_wrapped<Depth>(std::declval<T&>(), PIKA_FORWARD(Args, args)...))
        {
            return invoke_wrapped<Depth>(wrapped_, PIKA_FORWARD(Args, args)...);
        }

        template <typename... Args>
        auto operator()(Args&&... args) const -> decltype(invoke_wrapped<Depth>(
            std::declval<T const&>(), PIKA_FORWARD(Args, args)...))
        {
            return invoke_wrapped<Depth>(wrapped_, PIKA_FORWARD(Args, args)...);
        }
    };

    /// Returns a callable object which unwraps the futures
    /// contained in the given pack args until the depth Depth.
    template <std::size_t Depth, typename T>
    auto functional_unwrap_depth_impl(T&& callable)
        -> functional_unwrap_impl<std::decay_t<T>, Depth>
    {
        return functional_unwrap_impl<std::decay_t<T>, Depth>(PIKA_FORWARD(T, callable));
    }
}    // namespace pika::util::detail
