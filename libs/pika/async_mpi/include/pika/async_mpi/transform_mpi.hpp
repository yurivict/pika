//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/transform_xxx.hpp

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/async_mpi/mpi_polling.hpp>
#include <pika/concepts/concepts.hpp>
#include <pika/datastructures/variant.hpp>
#include <pika/debugging/demangle_helper.hpp>
#include <pika/debugging/print.hpp>
#include <pika/execution/algorithms/detail/helpers.hpp>
#include <pika/execution/algorithms/detail/partial_algorithm.hpp>
#include <pika/execution/algorithms/transfer.hpp>
#include <pika/execution_base/any_sender.hpp>
#include <pika/execution_base/receiver.hpp>
#include <pika/execution_base/sender.hpp>
#include <pika/executors/thread_pool_scheduler.hpp>
#include <pika/functional/detail/tag_fallback_invoke.hpp>
#include <pika/functional/invoke.hpp>
#include <pika/functional/invoke_fused.hpp>
#include <pika/mpi_base/mpi.hpp>

#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pika::mpi::experimental {
    namespace transform_mpi_detail {
        // -----------------------------------------------------------------
        // by convention the title is 7 chars (for alignment)
        using print_on = pika::debug::detail::enable_print<false>;
        inline constexpr print_on mpi_tran("MPITRAN");

        // -----------------------------------------------------------------
        // calls set_value or set_error on the receiver
        template <typename Receiver, typename... Ts>
        void set_value_request_callback_helper(int mpi_status, Receiver&& receiver, Ts&&... ts)
        {
            static_assert(sizeof...(Ts) <= 1, "Expecting at most one value");
            if (mpi_status == MPI_SUCCESS)
            {
                pika::execution::experimental::set_value(
                    PIKA_FORWARD(Receiver, receiver), PIKA_FORWARD(Ts, ts)...);
            }
            else
            {
                pika::execution::experimental::set_error(PIKA_FORWARD(Receiver, receiver),
                    std::make_exception_ptr(mpi_exception(mpi_status)));
            }
        }

        // -----------------------------------------------------------------
        // After an MPI call is made, a callback must be given to the polling
        // code to allow the result of the mpi call to be set when the request
        // has completed. This function sets the callback to invoke
        // the callback helper with or without a passed result.
        // (mpi calls nearly always return an int, so the void one is not used much)
        template <typename OperationState>
        void set_value_request_callback_void(MPI_Request request, OperationState& op_state)
        {
            detail::add_request_callback(
                [&op_state](int status) mutable {
                    using namespace pika::debug::detail;
                    PIKA_DP(mpi_tran,
                        debug(str<>("callback_void"), "stream",
                            detail::stream_name(op_state.stream)));
                    op_state.ts = {};
                    set_value_request_callback_helper(status, PIKA_MOVE(op_state.receiver));
                },
                request, detail::check_request_eager::yes, op_state.stream);
        }

        template <typename Result, typename OperationState>
        void set_value_request_callback_non_void(MPI_Request request, OperationState& op_state)
        {
            detail::add_request_callback(
                [&op_state](int status) mutable {
                    using namespace pika::debug::detail;
                    PIKA_DP(mpi_tran,
                        debug(str<>("callback_nonvoid"), "stream",
                            detail::stream_name(op_state.stream)));
                    op_state.ts = {};
                    PIKA_ASSERT(std::holds_alternative<Result>(op_state.result));
                    set_value_request_callback_helper(status, PIKA_MOVE(op_state.receiver),
                        PIKA_MOVE(std::get<Result>(op_state.result)));
                },
                request, detail::check_request_eager::yes, op_state.stream);
        }

        template <typename Result, typename OperationState>
        void
        set_value_request_callback_suspend_resume(MPI_Request request, OperationState& op_state)
        {
            detail::add_request_callback(
                [&op_state](int status) mutable {
                    using namespace pika::debug::detail;
                    PIKA_DP(mpi_tran,
                        debug(str<>("callback_void_suspend_resume"), "stream",
                            detail::stream_name(op_state.stream)));
                    op_state.ts = {};
                    op_state.status = status;

                    // wake up the suspended thread
                    {
                        std::lock_guard lk(op_state.mutex_);
                        op_state.completed = true;
                    }
                    op_state.cond_var_.notify_one();
                },
                // we do not need to eagerly check, because it was done earlier
                request, detail::check_request_eager::no, op_state.stream);
        }

        // -----------------------------------------------------------------
        // can function be invoked with param types + MPI_Request
        template <typename F, typename... Ts>
        inline constexpr bool is_mpi_request_invocable_v =
            std::is_invocable_v<F, std::add_lvalue_reference_t<std::decay_t<Ts>>..., MPI_Request*>;

        // -----------------------------------------------------------------
        // get return type of func(Ts..., MPI_Request)
        template <typename F, typename... Ts>
        using mpi_request_invoke_result_t = std::decay_t<std::invoke_result_t<F,
            std::add_lvalue_reference_t<std::decay_t<Ts>>..., MPI_Request*>>;

        // -----------------------------------------------------------------
        // route calls through an impl layer for ADL resolution
        template <typename Sender, typename F>
        struct transform_mpi_sender_impl
        {
            struct transform_mpi_sender_type;
        };

        template <typename Sender, typename F>
        using transform_mpi_sender =
            typename transform_mpi_sender_impl<Sender, F>::transform_mpi_sender_type;

        // -----------------------------------------------------------------
        // transform MPI adapter - sender type
        template <typename Sender, typename F>
        struct transform_mpi_sender_impl<Sender, F>::transform_mpi_sender_type
        {
            using is_sender = void;

            std::decay_t<Sender> sender;
            std::decay_t<F> f;
            stream_type stream;

#if defined(PIKA_HAVE_STDEXEC)
            template <typename... Ts>
            requires is_mpi_request_invocable_v<F, Ts...>
            using invoke_result_helper = pika::execution::experimental::completion_signatures<
                pika::execution::experimental::detail::result_type_signature_helper_t<
                    mpi_request_invoke_result_t<F, Ts...>>>;

            using completion_signatures =
                pika::execution::experimental::make_completion_signatures<std::decay_t<Sender>,
                    pika::execution::experimental::empty_env,
                    pika::execution::experimental::completion_signatures<
                        pika::execution::experimental::set_error_t(std::exception_ptr)>,
                    invoke_result_helper>;
#else
            // -----------------------------------------------------------------
            // get the return tuple<type> of func (tuple<args> + MPI_Request)
            template <typename Tuple>
            struct invoke_result_helper;

            template <template <typename...> class Tuple, typename... Ts>
            struct invoke_result_helper<Tuple<Ts...>>
            {
                static_assert(is_mpi_request_invocable_v<F, Ts...>,
                    "F not invocable with the value_types specified.");
                using result_type = mpi_request_invoke_result_t<F, Ts...>;
                using type = std::conditional_t<std::is_void<result_type>::value, Tuple<>,
                    Tuple<result_type>>;
            };

            // -----------------------------------------------------------------
            // get pack of unique types from combined tuple and variant
            template <template <typename...> class Tuple, template <typename...> class Variant>
            using value_types = pika::util::detail::unique_t<pika::util::detail::transform_t<
                typename pika::execution::experimental::sender_traits<Sender>::template value_types<
                    Tuple, Variant>,
                invoke_result_helper>>;

            template <template <typename...> class Variant>
            using error_types = pika::util::detail::unique_t<
                pika::util::detail::prepend_t<typename pika::execution::experimental::sender_traits<
                                                  Sender>::template error_types<Variant>,
                    std::exception_ptr>>;

            static constexpr bool sends_done = false;
#endif

            // -----------------------------------------------------------------
            // operation state for a given receiver
            template <typename Receiver>
            struct operation_state
            {
                std::decay_t<Receiver> receiver;
                std::decay_t<F> f;
                stream_type stream;
                pika::spinlock mutex_;
                pika::condition_variable cond_var_;
                bool completed;
                int status;

                // -----------------------------------------------------------------
                // The mpi_receiver receives inputs from the previous sender,
                // invokes the mpi call, and sets a callback on the polling handler
                struct transform_mpi_receiver
                {
                    using is_receiver = void;

                    operation_state& op_state;

                    template <typename Error>
                    friend constexpr void tag_invoke(pika::execution::experimental::set_error_t,
                        transform_mpi_receiver&& r, Error&& error) noexcept
                    {
                        pika::execution::experimental::set_error(
                            PIKA_MOVE(r.op_state.receiver), PIKA_FORWARD(Error, error));
                    }

                    friend constexpr void tag_invoke(pika::execution::experimental::set_stopped_t,
                        transform_mpi_receiver&& r) noexcept
                    {
                        pika::execution::experimental::set_stopped(PIKA_MOVE(r.op_state.receiver));
                    };

                    // receive the MPI function and arguments and add a request,
                    // then invoke the mpi function and set a callback to be
                    // triggered when the mpi request completes
                    template <typename... Ts,
                        typename = std::enable_if_t<is_mpi_request_invocable_v<F, Ts...>>>
                    friend constexpr void tag_invoke(pika::execution::experimental::set_value_t,
                        transform_mpi_receiver&& r, Ts&&... ts) noexcept
                    {
                        pika::detail::try_catch_exception_ptr(
                            [&]() mutable {
                                using namespace pika::debug::detail;
                                using ts_element_type = std::tuple<std::decay_t<Ts>...>;
                                using invoke_result_type = mpi_request_invoke_result_t<F, Ts...>;
                                //
                                r.op_state.ts.template emplace<ts_element_type>(
                                    PIKA_FORWARD(Ts, ts)...);
                                auto& t = std::get<ts_element_type>(r.op_state.ts);
                                //
                                MPI_Request request{MPI_REQUEST_NULL};
                                // modes 0 uses the task yield_while method of callback
                                // modes 1,2 use the task resume method of callback
                                auto mode = experimental::get_completion_mode();
                                if (mode < 3)
                                {
                                    pika::util::detail::invoke_fused(
                                        [&](auto&... ts) mutable {
                                            PIKA_DP(mpi_tran,
                                                debug(str<>("mpi invoke"), dec<2>(mode),
                                                    print_type<invoke_result_type>()));
                                            // execute the mpi function call, passing in the request object
                                            if constexpr (std::is_void_v<invoke_result_type>)
                                            {
                                                PIKA_INVOKE(
                                                    PIKA_MOVE(r.op_state.f), ts..., &request);
                                                PIKA_ASSERT_MSG(request != MPI_REQUEST_NULL,
                                                    "MPI_REQUEST_NULL returned from mpi "
                                                    "invocation");
                                            }
                                            else
                                            {
                                                r.op_state.result
                                                    .template emplace<invoke_result_type>(
                                                        PIKA_INVOKE(PIKA_MOVE(r.op_state.f), ts...,
                                                            &request));
                                                PIKA_ASSERT_MSG(request != MPI_REQUEST_NULL,
                                                    "MPI_REQUEST_NULL returned from mpi "
                                                    "invocation");
                                            }
                                        },
                                        t);
                                    //
                                    if (mode == 0)
                                    {
                                        pika::util::yield_while([&request]() {
                                            return !detail::poll_request(request);
                                        });
                                    }
                                    else
                                    {
                                        // don't suspend if request completed already
                                        if (!detail::poll_request(request))
                                        {
                                            set_value_request_callback_suspend_resume<
                                                invoke_result_type>(request, r.op_state);
                                            PIKA_ASSERT(pika::threads::detail::get_self_id());
                                            threads::detail::thread_data::scoped_thread_priority
                                                set_restore(pika::execution::thread_priority::high);
                                            std::unique_lock l{r.op_state.mutex_};
                                            r.op_state.cond_var_.wait(
                                                l, [&]() { return r.op_state.completed; });
                                        }
                                    }
                                    r.op_state.ts = {};
                                    r.op_state.status = MPI_SUCCESS;
                                    if constexpr (!std::is_void_v<invoke_result_type>)
                                    {
                                        set_value_request_callback_helper(r.op_state.status,
                                            PIKA_MOVE(r.op_state.receiver),
                                            PIKA_MOVE(
                                                std::get<invoke_result_type>(r.op_state.result)));
                                    }
                                    else
                                    {
                                        set_value_request_callback_helper(
                                            r.op_state.status, PIKA_MOVE(r.op_state.receiver));
                                    }
                                }
                                // modes 3,4,5,6,7,8 ....
                                else
                                {
                                    PIKA_DP(mpi_tran,
                                        debug(str<>("throttle?"), "stream",
                                            detail::stream_name(r.op_state.stream)));
                                    // throttle if too many "in flight"
                                    detail::wait_for_throttling(r.op_state.stream);
                                    PIKA_DP(mpi_tran,
                                        debug(str<>("mpi invoke"), dec<2>(mode),
                                            print_type<invoke_result_type>()));
                                    if constexpr (std::is_void_v<invoke_result_type>)
                                    {
                                        pika::util::detail::invoke_fused(
                                            [&](auto&... ts) mutable {
                                                PIKA_INVOKE(
                                                    PIKA_MOVE(r.op_state.f), ts..., &request);
                                                PIKA_ASSERT_MSG(request != MPI_REQUEST_NULL,
                                                    "MPI_REQUEST_NULL returned from mpi "
                                                    "invocation");
                                                // return type void, no value to forward to receiver
                                                set_value_request_callback_void(
                                                    request, r.op_state);
                                            },
                                            t);
                                    }
                                    else
                                    {
                                        pika::util::detail::invoke_fused(
                                            [&](auto&... ts) mutable {
                                                r.op_state.result
                                                    .template emplace<invoke_result_type>(
                                                        PIKA_INVOKE(PIKA_MOVE(r.op_state.f), ts...,
                                                            &request));
                                                PIKA_ASSERT_MSG(request != MPI_REQUEST_NULL,
                                                    "MPI_REQUEST_NULL returned from mpi "
                                                    "invocation");
                                                // forward value to receiver
                                                set_value_request_callback_non_void<
                                                    invoke_result_type>(request, r.op_state);
                                            },
                                            t);
                                    }
                                }
                            },
                            [&](std::exception_ptr ep) {
                                pika::execution::experimental::set_error(
                                    PIKA_MOVE(r.op_state.receiver), PIKA_MOVE(ep));
                            });
                    }

                    friend constexpr pika::execution::experimental::empty_env tag_invoke(
                        pika::execution::experimental::get_env_t,
                        transform_mpi_receiver const&) noexcept
                    {
                        return {};
                    }
                };

                using operation_state_type =
                    pika::execution::experimental::connect_result_t<std::decay_t<Sender>,
                        transform_mpi_receiver>;
                operation_state_type op_state;

                template <typename Tuple>
                struct value_types_helper
                {
                    using type = pika::util::detail::transform_t<Tuple, std::decay>;
                };

#if defined(PIKA_HAVE_STDEXEC)
                using ts_type = pika::util::detail::prepend_t<
                    pika::util::detail::transform_t<
                        pika::execution::experimental::value_types_of_t<std::decay_t<Sender>,
                            pika::execution::experimental::empty_env, std::tuple,
                            pika::detail::variant>,
                        value_types_helper>,
                    pika::detail::monostate>;
#else
                using ts_type = pika::util::detail::prepend_t<
                    pika::util::detail::transform_t<
                        typename pika::execution::experimental::sender_traits<std::decay_t<
                            Sender>>::template value_types<std::tuple, pika::detail::variant>,
                        value_types_helper>,
                    pika::detail::monostate>;
#endif
                ts_type ts;

                // We store the return value of f in a variant. We know that
                // value_types of the transform_mpi_sender contains packs of at
                // most one element (the return value of f), so we only
                // specialize result_types_helper for zero or one value. For
                // empty packs we use pika::detail::monostate since we don't
                // need to store anything in that case.
                //
                // All in all, we:
                // - transform one-element packs to the single element, and
                //   empty packs to pika::detail::monostate
                // - add pika::detail::monostate to the pack in case it wasn't
                //   there already
                // - remove duplicates in case pika::detail::monostate has been
                //   added twice
                // - change the outer pack to a pika::detail::variant
                template <typename Tuple>
                struct result_types_helper;

                template <template <typename...> class Tuple, typename T>
                struct result_types_helper<Tuple<T>>
                {
                    using type = std::decay_t<T>;
                };

                template <template <typename...> class Tuple>
                struct result_types_helper<Tuple<>>
                {
                    using type = pika::detail::monostate;
                };
#if defined(PIKA_HAVE_STDEXEC)
                using result_type = pika::util::detail::change_pack_t<pika::detail::variant,
                    pika::util::detail::unique_t<pika::util::detail::prepend_t<
                        pika::util::detail::transform_t<
                            pika::execution::experimental::value_types_of_t<
                                transform_mpi_sender_type, pika::execution::experimental::empty_env,
                                pika::util::detail::pack, pika::util::detail::pack>,
                            result_types_helper>,
                        pika::detail::monostate>>>;
#else
                using result_type = pika::util::detail::change_pack_t<pika::detail::variant,
                    pika::util::detail::unique_t<pika::util::detail::prepend_t<
                        pika::util::detail::transform_t<
                            transform_mpi_sender_type::value_types<pika::util::detail::pack,
                                pika::util::detail::pack>,
                            result_types_helper>,
                        pika::detail::monostate>>>;
#endif
                result_type result;

                template <typename Receiver_, typename F_, typename Sender_>
                operation_state(Receiver_&& receiver, F_&& f, Sender_&& sender, stream_type s)
                  : receiver(PIKA_FORWARD(Receiver_, receiver))
                  , f(PIKA_FORWARD(F_, f))
                  , stream{s}
                  , completed{false}
                  , status{MPI_SUCCESS}
                  , op_state(pika::execution::experimental::connect(
                        PIKA_FORWARD(Sender_, sender), transform_mpi_receiver{*this}))
                {
                    PIKA_DP(mpi_tran,
                        debug(debug::detail::str<>("operation_state"), "stream",
                            detail::stream_name(s)));
                }

                friend constexpr auto tag_invoke(
                    pika::execution::experimental::start_t, operation_state& os) noexcept
                {
                    return pika::execution::experimental::start(os.op_state);
                }
            };

            template <typename Receiver>
            friend constexpr auto tag_invoke(pika::execution::experimental::connect_t,
                transform_mpi_sender_type const& s, Receiver&& receiver)
            {
                return operation_state<Receiver>(
                    PIKA_FORWARD(Receiver, receiver), s.f, s.sender, s.stream);
            }

            template <typename Receiver>
            friend constexpr auto tag_invoke(pika::execution::experimental::connect_t,
                transform_mpi_sender_type&& s, Receiver&& receiver)
            {
                return operation_state<Receiver>(PIKA_FORWARD(Receiver, receiver), PIKA_MOVE(s.f),
                    PIKA_MOVE(s.sender), s.stream);
            }
        };
    }    // namespace transform_mpi_detail

    inline auto mpi_pool_scheduler()
    {
        using pika::execution::experimental::thread_pool_scheduler;
        return thread_pool_scheduler{&pika::resource::get_thread_pool(get_pool_name())};
    }
    inline auto default_pool_scheduler()
    {
        using pika::execution::experimental::thread_pool_scheduler;
        return thread_pool_scheduler{&pika::resource::get_thread_pool("default")};
    }

    template <typename T>
    struct any_sender_helper
    {
        using type = pika::execution::experimental::unique_any_sender<T>;
    };

    template <>
    struct any_sender_helper<void>
    {
        using type = pika::execution::experimental::unique_any_sender<>;
    };

    inline constexpr struct transform_mpi_t final
      : pika::functional::detail::tag_fallback<transform_mpi_t>
    {
    private:
        template <typename Sender, typename F,
            PIKA_CONCEPT_REQUIRES_(
                pika::execution::experimental::is_sender_v<std::decay_t<Sender>>)>
        friend constexpr PIKA_FORCEINLINE auto tag_fallback_invoke(transform_mpi_t, Sender&& sender,
            F&& f, stream_type s = stream_type::automatic)
        {
            using namespace transform_mpi_detail;
            PIKA_DP(mpi_tran,
                debug(
                    debug::detail::str<>("tag_fallback_invoke"), "stream", detail::stream_name(s)));

            using pika::execution::experimental::make_unique_any_sender;
            using pika::execution::experimental::thread_pool_scheduler;
            using pika::execution::experimental::transfer;
            using pika::execution::experimental::with_priority;
            using pika::execution::experimental::with_stacksize;

            // what is the output of set_value for this transform_mpi sender
            using our_type = transform_mpi_sender<Sender, F>;
            using value_type =
                typename std::decay<pika::execution::experimental::detail::single_result_t<
                    typename pika::execution::experimental::sender_traits<our_type>::
                        template value_types<pika::util::detail::pack, pika::util::detail::pack>>>::
                    type;
            // this is the final sender type unique_any_sender<?>
            typename any_sender_helper<value_type>::type result;

            // does a custom mpi pool exist?
            auto mpi_exist = pool_exists();
            // get the mpi completion mode
            auto mode = get_completion_mode();
            if (mode == 0)
            {
                // use yield_while on the mpi pool
                auto snd0 = PIKA_FORWARD(Sender, sender) | transfer(mpi_pool_scheduler());
                auto snd1 =
                    transform_mpi_sender<decltype(snd0), F>{PIKA_MOVE(snd0), PIKA_FORWARD(F, f), s};
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 1)
            {
                // run mpi inline
                // use suspend/resume without transfer of completion
                auto snd1 = transform_mpi_sender<Sender, F>{
                    PIKA_FORWARD(Sender, sender), PIKA_FORWARD(F, f), s};
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 2)
            {
                if (!mpi_exist)
                {
                    // same as mode 1
                    auto snd1 = transform_mpi_sender<Sender, F>{
                        PIKA_FORWARD(Sender, sender), PIKA_FORWARD(F, f), s};
                    result = make_unique_any_sender(std::move(snd1));
                }
                else
                {
                    // transfer to mpi pool and use suspend/resume there
                    auto snd0 = PIKA_FORWARD(Sender, sender) | transfer(mpi_pool_scheduler());
                    auto snd1 = transform_mpi_sender<decltype(snd0), F>{
                        std::move(snd0), PIKA_FORWARD(F, f), s};
                    result = make_unique_any_sender(std::move(snd1));
                }
            }
            // ----------------------------------------------------------
            else if (mode == 3)
            {
                // run mpi inline
                // run completion explicitly on default pool with High priority
                auto snd1 = transform_mpi_sender<Sender, F>{PIKA_FORWARD(Sender, sender),
                                PIKA_FORWARD(F, f), s} |
                    transfer(with_priority(
                        default_pool_scheduler(), pika::execution::thread_priority::high));
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 4)
            {
                // run mpi inline
                // run completion explicitly on default pool without priority
                auto snd1 = transform_mpi_sender<Sender, F>{PIKA_FORWARD(Sender, sender),
                                PIKA_FORWARD(F, f), s} |
                    transfer(default_pool_scheduler());
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 5)
            {
                // transfer mpi to mpi pool,
                // run completion explicitly on default pool with High priority
                auto snd0 = PIKA_FORWARD(Sender, sender) |
                    transfer(with_stacksize(
                        mpi_pool_scheduler(), pika::execution::thread_stacksize::nostack));
                auto snd1 = transform_mpi_sender<decltype(snd0), F>{std::move(snd0),
                                PIKA_FORWARD(F, f), s} |
                    transfer(with_priority(
                        default_pool_scheduler(), pika::execution::thread_priority::high));
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 6)
            {
                // transfer mpi to mpi pool,
                // run completion explicitly on default pool without priority
                auto snd0 = PIKA_FORWARD(Sender, sender) |
                    transfer(with_stacksize(
                        mpi_pool_scheduler(), pika::execution::thread_stacksize::nostack));
                auto snd1 = transform_mpi_sender<decltype(snd0), F>{std::move(snd0),
                                PIKA_FORWARD(F, f), s} |
                    transfer(default_pool_scheduler());
                result = make_unique_any_sender(std::move(snd1));
            }

            // ----------------------------
            // Modes need checking before use
            // ----------------------------
            else if (mode == 7)
            {
                // transfer mpi to mpi pool,
                // run completion on polling thread (mpi or default pool)
                auto snd0 = PIKA_FORWARD(Sender, sender) |
                    transfer(with_stacksize(
                        mpi_pool_scheduler(), pika::execution::thread_stacksize::nostack));
                auto snd1 =
                    transform_mpi_sender<decltype(snd0), F>{std::move(snd0), PIKA_FORWARD(F, f), s};
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 8)
            {
                // transfer mpi to mpi pool
                // run completion explicitly on mpi pool as high priority
                auto snd0 = PIKA_FORWARD(Sender, sender) |
                    transfer(with_stacksize(
                        mpi_pool_scheduler(), pika::execution::thread_stacksize::nostack));
                auto snd1 = transform_mpi_sender<decltype(snd0), F>{std::move(snd0),
                                PIKA_FORWARD(F, f), s} |
                    transfer(with_priority(
                        mpi_pool_scheduler(), pika::execution::thread_priority::high));
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 9)
            {
                // transfer mpi to mpi pool
                // run completion explicitly on default pool using high priority
                auto snd0 = PIKA_FORWARD(Sender, sender) |
                    transfer(with_stacksize(
                        mpi_pool_scheduler(), pika::execution::thread_stacksize::nostack));
                auto snd1 = transform_mpi_sender<decltype(snd0), F>{std::move(snd0),
                                PIKA_FORWARD(F, f), s} |
                    transfer(with_priority(
                        default_pool_scheduler(), pika::execution::thread_priority::high));
                result = make_unique_any_sender(std::move(snd1));
            }
            else if (mode == 10)
            {
                // transfer mpi to mpi pool
                // run completion explicitly on default pool using default priority
                auto snd0 = PIKA_FORWARD(Sender, sender) |
                    transfer(with_stacksize(
                        mpi_pool_scheduler(), pika::execution::thread_stacksize::nostack));
                auto snd1 = transform_mpi_sender<decltype(snd0), F>{std::move(snd0),
                                PIKA_FORWARD(F, f), s} |
                    transfer(with_priority(
                        default_pool_scheduler(), pika::execution::thread_priority::normal));
                result = make_unique_any_sender(std::move(snd1));
            }
            /*
 *     Temporarily disabled until bypass scheduler available
 *
                else if (mode == 11)
                {
                    // run mpi inline on current pool
                    // run completion with bypass on mpi pool
                    auto snd1 =
                        transform_mpi_sender<Sender, F>{
                            PIKA_FORWARD(Sender, sender), PIKA_FORWARD(F, f),
                            s} |
                        transfer(thread_pool_scheduler_queue_bypass{
                            &pika::resource::get_thread_pool(
                                get_pool_name())});
                    return make_unique_any_sender(std::move(snd1));
                }
                else if (mode == 12)
                {
                    // transfer mpi to mpi pool,
                    // run completion with bypass on mpi pool
                    auto snd0 = PIKA_FORWARD(Sender, sender) |
                        transfer(with_stacksize(
                            mpi_pool_scheduler(),
                            pika::execution::thread_stacksize::nostack));
                    auto snd1 =
                        transform_mpi_sender<decltype(snd0), F>{
                            std::move(snd0), PIKA_FORWARD(F, f), s} |
                        transfer(thread_pool_scheduler_queue_bypass{
                            &pika::resource::get_thread_pool(
                                get_pool_name())});
                    return make_unique_any_sender(std::move(snd1));
                }
                else if (mode == 13)
                {
                    // transfer mpi to mpi pool
                    // run completion inline with bypass on default pool
                    // only effective if default pool is polling pool (mpi=default)
                    auto snd0 = PIKA_FORWARD(Sender, sender) |
                        transfer(with_stacksize(
                            mpi_pool_scheduler(),
                            pika::execution::thread_stacksize::nostack));
                    auto snd1 =
                        transform_mpi_sender<decltype(snd0), F>{
                            std::move(snd0), PIKA_FORWARD(F, f), s} |
                        transfer(thread_pool_scheduler_queue_bypass{
                            &pika::resource::get_thread_pool("default")});
                    return make_unique_any_sender(std::move(snd1));
                }
*/
            else
            {
                PIKA_THROW_EXCEPTION(pika::error::bad_parameter, "transform_mpi",
                    "Unsupported transfer mode: {} (valid options are between {} and {} and "
                    "can be set with env{PIKA_MPI_COMPLETION_MODE}",
                    mode, 0, 10);
            }

            if constexpr (pika::execution::experimental::detail::has_completion_scheduler_v<
                              pika::execution::experimental::set_value_t, std::decay_t<Sender>>)
            {
                auto cs = pika::execution::experimental::get_completion_scheduler<
                    pika::execution::experimental::set_value_t>(PIKA_FORWARD(Sender, sender));
                result = make_unique_any_sender(transfer(cs, std::move(result)));
                throw std::runtime_error("This is never called!");
            }
            return result;
        }

        //
        // tag invoke overload for mpi_transform
        //
        template <typename F>
        friend constexpr PIKA_FORCEINLINE auto
        tag_fallback_invoke(transform_mpi_t, F&& f, stream_type s = stream_type::automatic)
        {
            return ::pika::execution::experimental::detail::partial_algorithm<transform_mpi_t, F,
                stream_type>{PIKA_FORWARD(F, f), s};
        }

    } transform_mpi{};
}    // namespace pika::mpi::experimental
