//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/allocator_support/allocator_deleter.hpp>
#include <pika/assert.hpp>
#include <pika/functional/detail/invoke.hpp>
#include <pika/functional/invoke_fused.hpp>
#include <pika/futures/traits/future_access.hpp>
#include <pika/memory/intrusive_ptr.hpp>
#include <pika/pack_traversal/detail/container_category.hpp>
#include <pika/type_support/decay.hpp>
#include <pika/type_support/pack.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace pika {
    namespace util::detail {
        /// A tag which is passed to the `operator()` of the visitor
        /// if an element is visited synchronously.
        struct async_traverse_visit_tag
        {
        };

        /// A tag which is passed to the `operator()` of the visitor
        /// if an element is visited after the traversal was detached.
        struct async_traverse_detach_tag
        {
        };

        /// A tag which is passed to the `operator()` of the visitor
        /// if the asynchronous pack traversal was finished.
        struct async_traverse_complete_tag
        {
        };

        /// A tag to identify that a mapper shall be constructed in-place
        /// from the first argument passed.
        template <typename T>
        struct async_traverse_in_place_tag
        {
        };

        /// Relocates the given pack with the given offset
        template <std::size_t Offset, typename Pack>
        struct relocate_index_pack;
        template <std::size_t Offset, std::size_t... Sequence>
        struct relocate_index_pack<Offset, index_pack<Sequence...>>
          : std::common_type<index_pack<(Sequence + Offset)...>>
        {
        };

        /// Creates a sequence from begin to end explicitly
        template <std::size_t Begin, std::size_t End>
        using explicit_range_sequence_of_t =
            typename relocate_index_pack<Begin, typename make_index_pack<End - Begin>::type>::type;

        /// Continues the traversal when the object is called
        template <typename Frame, typename State>
        class resume_traversal_callable
        {
            Frame frame_;
            State state_;

        public:
            explicit resume_traversal_callable(Frame frame, State state)
              : frame_(PIKA_MOVE(frame))
              , state_(PIKA_MOVE(state))
            {
            }

            /// The callable operator for resuming
            /// the asynchronous pack traversal
            void operator()();
        };

        /// Creates a resume_traversal_callable from the given frame and the
        /// given iterator tuple.
        template <typename Frame, typename State>
        auto make_resume_traversal_callable(Frame&& frame, State&& state)
            -> resume_traversal_callable<std::decay_t<Frame>, std::decay_t<State>>
        {
            return resume_traversal_callable<std::decay_t<Frame>, std::decay_t<State>>(
                PIKA_FORWARD(Frame, frame), PIKA_FORWARD(State, state));
        }

        /// Stores the visitor and the arguments to traverse
        template <typename Visitor, typename... Args>
        class async_traversal_frame : public Visitor
        {
        protected:
            std::tuple<Args...> args_;
            std::atomic<bool> finished_;

            Visitor& visitor() noexcept
            {
                return *static_cast<Visitor*>(this);
            }

            Visitor const& visitor() const noexcept
            {
                return *static_cast<Visitor const*>(this);
            }

        public:
            explicit async_traversal_frame(Visitor visitor, Args... args)
              : Visitor(PIKA_MOVE(visitor))
              , args_(std::make_tuple(PIKA_MOVE(args)...))
              , finished_(false)
            {
            }

            /// We require a virtual base
            ~async_traversal_frame() override
            {
                PIKA_ASSERT(finished_);
            }

            template <typename MapperArg>
            explicit async_traversal_frame(
                async_traverse_in_place_tag<Visitor>, MapperArg&& mapper_arg, Args... args)
              : Visitor(PIKA_FORWARD(MapperArg, mapper_arg))
              , args_(std::make_tuple(PIKA_MOVE(args)...))
              , finished_(false)
            {
            }

            /// Returns the arguments of the frame
            std::tuple<Args...>& head() noexcept
            {
                return args_;
            }

            /// Calls the visitor with the given element
            template <typename T>
            auto traverse(T&& value) -> std::invoke_result_t<Visitor&, async_traverse_visit_tag, T>
            {
                return PIKA_INVOKE(visitor(), async_traverse_visit_tag{}, PIKA_FORWARD(T, value));
            }

            /// Calls the visitor with the given element and a continuation
            /// which is capable of continuing the asynchronous traversal
            /// when it's called later.
            template <typename T, typename Hierarchy>
            void async_continue(T&& value, Hierarchy&& hierarchy)
            {
                // Create a self reference
                pika::intrusive_ptr<async_traversal_frame> self(this);

                // Create a callable object which resumes the current
                // traversal when it's called.
                auto resumable = make_resume_traversal_callable(
                    PIKA_MOVE(self), PIKA_FORWARD(Hierarchy, hierarchy));

                // Invoke the visitor with the current value and the
                // callable object to resume the control flow.
                PIKA_INVOKE(visitor(), async_traverse_detach_tag{}, PIKA_FORWARD(T, value),
                    PIKA_MOVE(resumable));
            }

            /// Calls the visitor with no arguments to signalize that the
            /// asynchronous traversal was finished.
            void async_complete()
            {
                bool expected = false;
#if defined(PIKA_GCC_VERSION) && PIKA_GCC_VERSION >= 120000
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
                if (finished_.compare_exchange_strong(expected, true))
#if defined(PIKA_GCC_VERSION) && PIKA_GCC_VERSION >= 120000
# pragma GCC diagnostic pop
#endif
                {
                    PIKA_INVOKE(visitor(), async_traverse_complete_tag{}, PIKA_MOVE(args_));
                }
            }
        };

        /// Stores the visitor and the arguments to traverse
        template <typename Allocator, typename Visitor, typename... Args>
        class async_traversal_frame_allocator : public async_traversal_frame<Visitor, Args...>
        {
            using base_type = async_traversal_frame<Visitor, Args...>;
            using other_allocator = typename std::allocator_traits<
                Allocator>::template rebind_alloc<async_traversal_frame_allocator>;

        public:
            explicit async_traversal_frame_allocator(
                other_allocator const& alloc, Visitor visitor, Args... args)
              : base_type(PIKA_MOVE(visitor), PIKA_MOVE(args)...)
              , alloc_(alloc)
            {
            }

            template <typename MapperArg>
            explicit async_traversal_frame_allocator(other_allocator const& alloc,
                async_traverse_in_place_tag<Visitor> tag, MapperArg&& mapper_arg, Args... args)
              : base_type(tag, PIKA_FORWARD(MapperArg, mapper_arg), PIKA_MOVE(args)...)
              , alloc_(alloc)
            {
            }

        private:
            void destroy() noexcept override
            {
                using traits = std::allocator_traits<other_allocator>;

                other_allocator alloc(alloc_);
                traits::destroy(alloc, this);
                traits::deallocate(alloc, this, 1);
            }

            other_allocator alloc_;
        };
    }    // namespace util::detail

    namespace traits::detail {
        template <typename Visitor, typename... Args, typename Allocator>
        struct shared_state_allocator<util::detail::async_traversal_frame<Visitor, Args...>,
            Allocator>
        {
            using type = util::detail::async_traversal_frame_allocator<Allocator, Visitor, Args...>;
        };
    }    // namespace traits::detail

    namespace util::detail {
        template <typename Target, std::size_t Begin, std::size_t End>
        struct static_async_range
        {
            Target* target_;

            explicit static_async_range(Target* target)
              : target_(target)
            {
            }

            static_async_range(static_async_range const& rhs) = default;
            static_async_range(static_async_range&& rhs)
              : target_(rhs.target_)
            {
                rhs.target_ = nullptr;
            }

            static_async_range& operator=(static_async_range const& rhs) = default;
            static_async_range& operator=(static_async_range&& rhs)
            {
                if (&rhs != this)
                {
                    target_ = rhs.target_;
                    rhs.target_ = nullptr;
                }
                return *this;
            }

            constexpr auto operator*() const noexcept -> decltype(std::get<Begin>(*target_))
            {
                return std::get<Begin>(*target_);
            }

            template <std::size_t Position>
            constexpr static_async_range<Target, Position, End> relocate() const noexcept
            {
                return static_async_range<Target, Position, End>{target_};
            }

            constexpr static_async_range<Target, Begin + 1, End> next() const noexcept
            {
                return static_async_range<Target, Begin + 1, End>{target_};
            }

            constexpr bool is_finished() const noexcept
            {
                return false;
            }
        };

        /// Specialization for the end marker which doesn't provide
        /// a particular element dereference
        template <typename Target, std::size_t Begin>
        struct static_async_range<Target, Begin, Begin>
        {
            explicit constexpr static_async_range(Target*) {}

            constexpr bool is_finished() const noexcept
            {
                return true;
            }
        };

        /// Returns a static range for the given type
        template <typename T,
            typename Range =
                static_async_range<std::decay_t<T>, 0U, std::tuple_size<std::decay_t<T>>::value>>
        Range make_static_range(T&& element)
        {
            auto pointer = std::addressof(element);
            return Range{pointer};
        }

        template <typename Begin, typename Sentinel>
        struct dynamic_async_range
        {
            Begin begin_;
            Sentinel sentinel_;

            dynamic_async_range& operator++() noexcept
            {
                ++begin_;
                return *this;
            }

            auto operator*() const noexcept -> decltype(*std::declval<Begin const&>())
            {
                return *begin_;
            }

            dynamic_async_range next() const
            {
                dynamic_async_range other = *this;
                ++other;
                return other;
            }

            bool is_finished() const
            {
                return begin_ == sentinel_;
            }
        };

        template <typename T>
        using dynamic_async_range_of_t =
            dynamic_async_range<std::decay_t<decltype(std::begin(std::declval<T>()))>,
                std::decay_t<decltype(std::end(std::declval<T>()))>>;

        /// Returns a dynamic range for the given type
        template <typename T, typename Range = dynamic_async_range_of_t<T>>
        Range make_dynamic_async_range(T&& element)
        {
            return Range{std::begin(element), std::end(element)};
        }

        template <typename T, typename Range = dynamic_async_range_of_t<T>>
        Range make_dynamic_async_range(std::reference_wrapper<T> ref_element)
        {
            return Range{std::begin(ref_element.get()), std::end(ref_element.get())};
        }

        /// Represents a particular point in a asynchronous traversal hierarchy
        template <typename Frame, typename... Hierarchy>
        class async_traversal_point
        {
            Frame frame_;
            std::tuple<Hierarchy...> hierarchy_;
            bool& detached_;

        public:
            explicit async_traversal_point(
                Frame frame, std::tuple<Hierarchy...> hierarchy, bool& detached)
              : frame_(PIKA_MOVE(frame))
              , hierarchy_(PIKA_MOVE(hierarchy))
              , detached_(detached)
            {
            }

            // Abort the current control flow
            void detach() noexcept
            {
                PIKA_ASSERT(!detached_);
                detached_ = true;
            }

            /// Returns true when we should abort the current control flow
            bool is_detached() const noexcept
            {
                return detached_;
            }

            /// Creates a new traversal point which
            template <typename Parent>
            auto push(Parent&& parent)
                -> async_traversal_point<Frame, std::decay_t<Parent>, Hierarchy...>
            {
                // Create a new hierarchy which contains the
                // the parent (the last traversed element).
                auto hierarchy =
                    std::tuple_cat(std::make_tuple(PIKA_FORWARD(Parent, parent)), hierarchy_);

                return async_traversal_point<Frame, std::decay_t<Parent>, Hierarchy...>(
                    frame_, PIKA_MOVE(hierarchy), detached_);
            }

            /// Forks the current traversal point and continues the child
            /// of the given parent.
            template <typename Child, typename Parent>
            void fork(Child&& child, Parent&& parent)
            {
                // Push the parent on top of the hierarchy
                auto point = push(PIKA_FORWARD(Parent, parent));

                // Continue the traversal with the current element
                point.async_traverse(PIKA_FORWARD(Child, child));
            }

            /// Async traverse a single element, and do nothing.
            /// This function is matched last.
            template <typename Matcher, typename Current>
            void async_traverse_one_impl(Matcher, Current&&)
            {
                // Do nothing if the visitor doesn't accept the type
            }

            /// Async traverse a single element which isn't a container or
            /// tuple like type. This function is SFINAEd out if the element
            /// isn't accepted by the visitor.
            template <typename Current,
                typename = std::void_t<decltype(std::declval<Frame>()->traverse(
                    *std::declval<Current>()))>>
            void async_traverse_one_impl(container_category_tag<false, false>, Current&& current)
            /// SFINAE this out if the visitor doesn't accept
            /// the given element
            {
                if (!frame_->traverse(*current))
                {
                    // Store the current call hierarchy into a tuple for
                    // later re-entrance.
                    auto hierarchy = std::tuple_cat(std::make_tuple(current.next()), hierarchy_);

                    // First detach the current execution context
                    detach();

                    // If the traversal method returns false, we detach the
                    // current execution context and call the visitor with the
                    // element and a continue callable object again.

                    frame_->async_continue(*current, PIKA_MOVE(hierarchy));
                }
            }

            /// Async traverse a single element which is a container or
            /// tuple like type.
            template <bool IsTupleLike, typename Current>
            void
            async_traverse_one_impl(container_category_tag<true, IsTupleLike>, Current&& current)
            {
                auto range = make_dynamic_async_range(*current);
                fork(PIKA_MOVE(range), PIKA_FORWARD(Current, current));
            }

            /// Async traverse a single element which is a tuple like type only.
            template <typename Current>
            void async_traverse_one_impl(container_category_tag<false, true>, Current&& current)
            {
                auto range = make_static_range(*current);
                fork(PIKA_MOVE(range), PIKA_FORWARD(Current, current));
            }

            /// Async traverse the current iterator
            template <typename Current>
            void async_traverse_one(Current&& current)
            {
                using ElementType = typename pika::detail::decay_unwrap<decltype(*current)>::type;
                return async_traverse_one_impl(
                    container_category_of_t<ElementType>{}, PIKA_FORWARD(Current, current));
            }

            /// Async traverse the current iterator but don't traverse
            /// if the control flow was detached.
            template <typename Current>
            void async_traverse_one_checked(Current&& current)
            {
                if (!is_detached())
                {
                    async_traverse_one(PIKA_FORWARD(Current, current));
                }
            }

            template <std::size_t... Sequence, typename Current>
            void async_traverse_static_async_range(index_pack<Sequence...>, Current&& current)
            {
                int dummy[] = {
                    ((void) async_traverse_one_checked(current.template relocate<Sequence>()),
                        0)...,
                    0};
                (void) dummy;
            }

            /// Traverse a static range
            template <typename Target, std::size_t Begin, std::size_t End>
            void async_traverse(static_async_range<Target, Begin, End> current)
            {
                async_traverse_static_async_range(
                    explicit_range_sequence_of_t<Begin, End>{}, current);
            }

            /// Traverse a dynamic range
            template <typename Begin, typename Sentinel>
            void async_traverse(dynamic_async_range<Begin, Sentinel> range)
            {
                if (!is_detached())
                {
                    for (/**/; !range.is_finished(); ++range)
                    {
                        async_traverse_one(range);
                        if (is_detached())    // test before increment
                            break;
                    }
                }
            }
        };

        /// Deduces to the traversal point class of the
        /// given frame and hierarchy
        template <typename Frame, typename... Hierarchy>
        using traversal_point_of_t =
            async_traversal_point<std::decay_t<Frame>, std::decay_t<Hierarchy>...>;

        /// A callable object which is capable of resuming an asynchronous
        /// pack traversal.
        struct resume_state_callable
        {
            /// Reenter an asynchronous iterator pack and continue
            /// its traversal.
            template <typename Frame, typename Current, typename... Hierarchy>
            void operator()(Frame&& frame, Current&& current, Hierarchy&&... hierarchy) const
            {
                bool detached = false;
                next(detached, PIKA_FORWARD(Frame, frame), PIKA_FORWARD(Current, current),
                    PIKA_FORWARD(Hierarchy, hierarchy)...);
            }

            template <typename Frame, typename Current>
            void next(bool& detached, Frame&& frame, Current&& current) const
            {
                // Only process the next element if the current iterator
                // hasn't reached its end.
                if (!current.is_finished())
                {
                    traversal_point_of_t<Frame> point(frame, std::make_tuple(), detached);

                    point.async_traverse(PIKA_FORWARD(Current, current));

                    // Don't continue the frame when the execution was detached
                    if (detached)
                    {
                        return;
                    }
                }

                frame->async_complete();
            }

            /// Reenter an asynchronous iterator pack and continue
            /// its traversal.
            template <typename Frame, typename Current, typename Parent, typename... Hierarchy>
            void next(bool& detached, Frame&& frame, Current&& current, Parent&& parent,
                Hierarchy&&... hierarchy) const
            {
                // Only process the element if the current iterator
                // hasn't reached its end.
                if (!current.is_finished())
                {
                    // Don't forward the arguments here, since we still need
                    // the objects in a valid state later.
                    traversal_point_of_t<Frame, Parent, Hierarchy...> point(
                        frame, std::make_tuple(parent, hierarchy...), detached);

                    point.async_traverse(PIKA_FORWARD(Current, current));

                    // Don't continue the frame when the execution was detached
                    if (detached)
                    {
                        return;
                    }
                }

                // Pop the top element from the hierarchy, and shift the
                // parent element one to the right
                next(detached, PIKA_FORWARD(Frame, frame),
#if defined(PIKA_CUDA_VERSION)
                    std::forward<Parent>(parent).next(),
#else
                    PIKA_FORWARD(Parent, parent).next(),
#endif
                    PIKA_FORWARD(Hierarchy, hierarchy)...);
            }
        };

        template <typename Frame, typename State>
        void resume_traversal_callable<Frame, State>::operator()()
        {
            auto hierarchy = std::tuple_cat(std::make_tuple(frame_), state_);
            util::detail::invoke_fused(resume_state_callable{}, PIKA_MOVE(hierarchy));
        }

        /// Gives access to types related to the traversal frame
        template <typename Visitor, typename... Args>
        struct async_traversal_types
        {
            /// Deduces to the async traversal frame type of the given
            /// traversal arguments and mapper
            using frame_type = async_traversal_frame<std::decay_t<Visitor>, std::decay_t<Args>...>;

            /// The type of the frame pointer
            using frame_pointer_type = pika::intrusive_ptr<frame_type>;

            /// The type of the demoted visitor type
            using visitor_pointer_type = pika::intrusive_ptr<Visitor>;
        };

        template <typename Visitor, typename VisitorArg, typename... Args>
        struct async_traversal_types<async_traverse_in_place_tag<Visitor>, VisitorArg, Args...>
          : async_traversal_types<Visitor, Args...>
        {
        };

        /// Traverses the given pack with the given mapper
        template <typename Visitor, typename... Args,
            typename types = async_traversal_types<Visitor, Args...>>
        auto apply_pack_transform_async(Visitor&& visitor, Args&&... args) ->
            typename types::visitor_pointer_type
        {
            // Create the frame on the heap which stores the arguments
            // to traverse asynchronously.
            //
            // Create an intrusive_ptr without increasing its reference count
            // (it's already 'one').
            auto frame = typename types::frame_pointer_type(new
                typename types::frame_type(
                    PIKA_FORWARD(Visitor, visitor), PIKA_FORWARD(Args, args)...),
                false);

            // Create a static range for the top level tuple
            auto range = make_static_range(frame->head());

            auto resumer = make_resume_traversal_callable(frame, std::make_tuple(PIKA_MOVE(range)));

            // Start the asynchronous traversal
            resumer();
            return frame;
        }

        /// Traverses the given pack with the given mapper, uses given allocator
        /// to allocate the traversal frame
        template <typename Allocator, typename Visitor, typename... Args,
            typename types = async_traversal_types<Visitor, Args...>>
        auto apply_pack_transform_async_allocator(Allocator const& a, Visitor&& visitor,
            Args&&... args) -> typename types::visitor_pointer_type
        {
            // Create the frame on the heap which stores the arguments
            // to traverse asynchronously.
            //
            // Create an intrusive_ptr without increasing its reference count
            // (it's already 'one').
            using shared_state =
                typename traits::detail::shared_state_allocator<typename types::frame_type,
                    Allocator>::type;

            using other_allocator =
                typename std::allocator_traits<Allocator>::template rebind_alloc<shared_state>;
            using traits = std::allocator_traits<other_allocator>;

            using unique_ptr =
                std::unique_ptr<shared_state, pika::detail::allocator_deleter<other_allocator>>;

            other_allocator frame_alloc(a);
            unique_ptr p(traits::allocate(frame_alloc, 1),
                pika::detail::allocator_deleter<other_allocator>{frame_alloc});
            traits::construct(frame_alloc, p.get(), frame_alloc, PIKA_FORWARD(Visitor, visitor),
                PIKA_FORWARD(Args, args)...);

            auto frame = typename types::frame_pointer_type(p.release(), false);

            // Create a static range for the top level tuple
            auto range = make_static_range(frame->head());

            auto resumer = make_resume_traversal_callable(frame, std::make_tuple(PIKA_MOVE(range)));

            // Start the asynchronous traversal
            resumer();
            return frame;
        }
    }    // namespace util::detail
}    // end namespace pika
