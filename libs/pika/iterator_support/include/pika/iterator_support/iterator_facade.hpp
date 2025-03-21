//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2016-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  This code is based on boost::iterators::iterator_facade
//  (C) Copyright David Abrahams 2002.
//  (C) Copyright Jeremy Siek    2002.
//  (C) Copyright Thomas Witt    2002.
//  (C) copyright Jeffrey Lee Hellrung, Jr. 2012.

#pragma once

#include <pika/config.hpp>
#include <pika/iterator_support/traits/is_iterator.hpp>

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace pika::util {
    ///////////////////////////////////////////////////////////////////////////
    // Helper class to gain access to the implementation functions in the
    // derived (user-defined) iterator classes.
    class iterator_core_access
    {
    public:
        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Iterator1, typename Iterator2>
        PIKA_HOST_DEVICE PIKA_FORCEINLINE static bool
        equal(Iterator1 const& lhs, Iterator2 const& rhs)
        {
            return lhs.equal(rhs);
        }

        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Iterator>
        PIKA_HOST_DEVICE PIKA_FORCEINLINE static void increment(Iterator& it)
        {
            it.increment();
        }

        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Iterator>
        PIKA_HOST_DEVICE PIKA_FORCEINLINE static void decrement(Iterator& it)
        {
            it.decrement();
        }

        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Reference, typename Iterator>
        PIKA_HOST_DEVICE PIKA_FORCEINLINE static Reference dereference(Iterator const& it)
        {
            return it.dereference();
        }

        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Iterator, typename Distance>
        PIKA_HOST_DEVICE PIKA_FORCEINLINE static void advance(Iterator& it, Distance n)
        {
            it.advance(n);
        }

        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Iterator1, typename Iterator2>
        PIKA_HOST_DEVICE PIKA_FORCEINLINE static
            typename std::iterator_traits<Iterator1>::difference_type
            distance_to(Iterator1 const& lhs, Iterator2 const& rhs)
        {
            return lhs.distance_to(rhs);
        }
    };

    namespace detail {
        ///////////////////////////////////////////////////////////////////////
        template <typename Reference>
        struct arrow_dispatch    // proxy references
        {
            struct proxy
            {
                PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
                PIKA_HOST_DEVICE
                explicit proxy(Reference const& x)
                  : ref_(x)
                {
                }
                PIKA_HOST_DEVICE PIKA_FORCEINLINE Reference* operator->()
                {
                    return std::addressof(ref_);
                }
                Reference ref_;
            };

            using type = proxy;

            PIKA_HOST_DEVICE PIKA_FORCEINLINE static type call(Reference const& x)
            {
                return type(x);
            }
        };

        template <typename T>
        struct arrow_dispatch<T&>    // "real" references
        {
            using type = T*;

            PIKA_HOST_DEVICE PIKA_FORCEINLINE static type call(T& x)
            {
                return std::addressof(x);
            }
        };

        template <typename T>
        using arrow_dispatch_t = typename arrow_dispatch<T>::type;

        ///////////////////////////////////////////////////////////////////////
        // Implementation for input and forward iterators
        template <typename Derived, typename T, typename Category, typename Reference,
            typename Distance, typename Pointer>
        class iterator_facade_base
        {
        public:
            using iterator_category = Category;
            using value_type = std::remove_const_t<T>;
            using difference_type = Distance;
            using pointer =
                std::conditional_t<std::is_void_v<Pointer>, arrow_dispatch_t<Reference>, Pointer>;
            using reference = Reference;

            iterator_facade_base() = default;

        protected:
            PIKA_HOST_DEVICE Derived& derived()
            {
                return *static_cast<Derived*>(this);
            }

            PIKA_HOST_DEVICE Derived const& derived() const
            {
                return *static_cast<Derived const*>(this);
            }

        public:
            PIKA_HOST_DEVICE reference operator*() const
            {
                return iterator_core_access::template dereference<reference>(this->derived());
            }

            PIKA_HOST_DEVICE pointer operator->() const
            {
                return arrow_dispatch<Reference>::call(*this->derived());
            }

            PIKA_HOST_DEVICE Derived& operator++()
            {
                Derived& this_ = this->derived();
                iterator_core_access::increment(this_);
                return this_;
            }
        };

        ////////////////////////////////////////////////////////////////////////
        // Implementation for bidirectional iterators
        template <typename Derived, typename T, typename Reference, typename Distance,
            typename Pointer>
        class iterator_facade_base<Derived, T, std::bidirectional_iterator_tag, Reference, Distance,
            Pointer>
          : public iterator_facade_base<Derived, T, std::forward_iterator_tag, Reference, Distance,
                Pointer>
        {
            using base_type = iterator_facade_base<Derived, T, std::forward_iterator_tag, Reference,
                Distance, Pointer>;

        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::remove_const_t<T>;
            using difference_type = Distance;
            using pointer =
                std::conditional_t<std::is_void_v<Pointer>, arrow_dispatch_t<Reference>, Pointer>;
            using reference = Reference;

            iterator_facade_base() = default;

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE Derived& operator--()
            {
                Derived& this_ = this->derived();
                iterator_core_access::decrement(this_);
                return this_;
            }

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE Derived operator--(int)
            {
                Derived result(this->derived());
                --*this;
                return result;
            }
        };

        ////////////////////////////////////////////////////////////////////////
        // Implementation for random access iterators

        // A proxy return type for operator[], needed to deal with
        // iterators that may invalidate referents upon destruction.
        // Consider the temporary iterator in *(a + n)
        template <typename Iterator>
        class operator_brackets_proxy
        {
            // Iterator is actually an iterator_facade, so we do not have to
            // go through iterator_traits to access the traits.
            using reference = typename Iterator::reference;
            using value_type = typename Iterator::value_type;

        public:
            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE explicit operator_brackets_proxy(Iterator const& iter) noexcept
              : iter_(iter)
            {
            }

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE operator reference() const
            {
                return *iter_;
            }

            PIKA_HOST_DEVICE operator_brackets_proxy& operator=(value_type const& val)
            {
                *iter_ = val;
                return *this;
            }

        private:
            Iterator iter_;
        };

        // A meta-function that determines whether operator[] must return a
        // proxy, or whether it can simply return a copy of the value_type.
        template <typename ValueType>
        struct use_operator_brackets_proxy
          : std::integral_constant<bool,
                !(std::is_copy_constructible_v<ValueType> && std::is_const_v<ValueType>)>
        {
        };

        template <typename Iterator, typename Value>
        struct operator_brackets_result
        {
            using type = std::conditional_t<use_operator_brackets_proxy<Value>::value,
                operator_brackets_proxy<Iterator>, Value>;
        };

        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Iterator>
        PIKA_HOST_DEVICE operator_brackets_proxy<Iterator>
        make_operator_brackets_result(Iterator const& iter, std::true_type)
        {
            return operator_brackets_proxy<Iterator>(iter);
        }

        PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
        template <typename Iterator>
        PIKA_HOST_DEVICE typename Iterator::value_type
        make_operator_brackets_result(Iterator const& iter, std::false_type)
        {
            return *iter;
        }

        template <typename Derived, typename T, typename Reference, typename Distance,
            typename Pointer>
        class iterator_facade_base<Derived, T, std::random_access_iterator_tag, Reference, Distance,
            Pointer>
          : public iterator_facade_base<Derived, T, std::bidirectional_iterator_tag, Reference,
                Distance, Pointer>
        {
            using base_type = iterator_facade_base<Derived, T, std::bidirectional_iterator_tag,
                Reference, Distance, Pointer>;

        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = std::remove_const_t<T>;
            using difference_type = Distance;
            using pointer =
                std::conditional_t<std::is_void_v<Pointer>, arrow_dispatch_t<Reference>, Pointer>;
            using reference = Reference;

            iterator_facade_base() = default;

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE
            typename operator_brackets_result<Derived, T>::type operator[](difference_type n) const
            {
                using use_proxy = use_operator_brackets_proxy<T>;

                return make_operator_brackets_result<Derived>(this->derived() + n, use_proxy{});
            }

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE Derived& operator+=(difference_type n)
            {
                Derived& this_ = this->derived();
                iterator_core_access::advance(this_, n);
                return this_;
            }

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE Derived operator+(difference_type n) const
            {
                Derived result(this->derived());
                return result += n;
            }

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE Derived& operator-=(difference_type n)
            {
                Derived& this_ = this->derived();
                iterator_core_access::advance(this_, -n);
                return this_;
            }

            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE Derived operator-(difference_type n) const
            {
                Derived result(this->derived());
                return result -= n;
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Derived, typename T, typename Category, typename Reference = T&,
        typename Distance = std::ptrdiff_t, typename Pointer = void>
    struct iterator_facade
      : detail::iterator_facade_base<Derived, T, Category, Reference, Distance, Pointer>
    {
    private:
        using base_type =
            detail::iterator_facade_base<Derived, T, Category, Reference, Distance, Pointer>;

    protected:
        // for convenience in derived classes
        using iterator_adaptor_ =
            iterator_facade<Derived, T, Category, Reference, Distance, Pointer>;

    public:
        using iterator_category = typename base_type::iterator_category;
        using value_type = typename base_type::value_type;
        using difference_type = typename base_type::difference_type;
        using pointer = typename base_type::pointer;
        using reference = typename base_type::reference;

        iterator_facade() = default;
    };

    namespace detail {
        // Iterators whose dereference operators reference the same value for
        // all iterators into the same sequence (like many input iterators)
        // need help with their postfix ++: the referenced value must be read
        // and stored away before the increment occurs so that *a++ yields the
        // originally referenced element and not the next one.
        template <typename Iterator>
        class postfix_increment_proxy
        {
            using value_type = typename std::iterator_traits<Iterator>::value_type;

        public:
            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE explicit postfix_increment_proxy(Iterator const& x)
              : stored_value(*x)
            {
            }

            // Returning a mutable reference allows nonsense like (*r++).mutate(),
            // but it imposes fewer assumptions about the behavior of the
            // value_type. In particular, recall that (*r).mutate() is legal if
            // operator* returns by value.
            PIKA_HOST_DEVICE value_type& operator*() const
            {
                return this->stored_value;
            }

        private:
            mutable std::remove_const_t<value_type> stored_value;
        };

        // In general, we can't determine that such an iterator isn't writable
        // -- we also need to store a copy of the old iterator so that it can
        // be written into.
        template <typename Iterator>
        class writable_postfix_increment_proxy
        {
            using value_type = typename std::iterator_traits<Iterator>::value_type;

        public:
            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE
            explicit writable_postfix_increment_proxy(Iterator const& x)
              : stored_value(*x)
              , stored_iterator(x)
            {
            }

            // Dereferencing must return a proxy so that both *r++ = o and
            // value_type(*r++) can work.  In this case, *r is the same as *r++,
            // and the conversion operator below is used to ensure readability.
            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            PIKA_HOST_DEVICE
            writable_postfix_increment_proxy const& operator*() const
            {
                return *this;
            }

            // Provides readability of *r++
            PIKA_HOST_DEVICE operator value_type&() const
            {
                return stored_value;
            }

            // Provides writability of *r++
            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            template <typename T>
            PIKA_HOST_DEVICE T const& operator=(T const& x) const
            {
                *this->stored_iterator = x;
                return x;
            }

            // This overload just in case only non-const objects are writable
            PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
            template <typename T>
            PIKA_HOST_DEVICE T& operator=(T& x) const
            {
                *this->stored_iterator = x;
                return x;
            }

            // Provides X(r++)
            PIKA_HOST_DEVICE operator Iterator const&() const
            {
                return stored_iterator;
            }

        private:
            mutable std::remove_const_t<value_type> stored_value;
            Iterator stored_iterator;
        };

        template <typename Reference, typename Value>
        struct is_non_proxy_reference
          : std::is_convertible<std::remove_reference_t<Reference> const volatile*,
                Value const volatile*>
        {
        };

        template <typename Reference, typename Value>
        inline constexpr bool is_non_proxy_reference_v =
            is_non_proxy_reference<Reference, Value>::value;

        // Because the C++98 input iterator requirements say that *r++ has
        // type T (value_type), implementations of some standard algorithms
        // like lexicographical_compare may use constructions like:
        //
        //          *r++ < *s++
        //
        // If *r++ returns a proxy (as required if r is writable but not
        // multi-pass), this sort of expression will fail unless the proxy
        // supports the operator<.  Since there are any number of such
        // operations, we're not going to try to support them.  Therefore,
        // even if r++ returns a proxy, *r++ will only return a proxy if *r
        // also returns a proxy.
        template <typename Iterator, typename Value, typename Reference, typename Enable = void>
        struct postfix_increment_result
        {
            using type = Iterator;
        };

        template <typename Iterator, typename Value, typename Reference>
        struct postfix_increment_result<Iterator, Value, Reference,
            std::enable_if_t<traits::has_category_v<Iterator, std::input_iterator_tag> &&
                is_non_proxy_reference_v<Reference, Value>>>
        {
            using type = postfix_increment_proxy<Iterator>;
        };

        template <typename Iterator, typename Value, typename Reference>
        struct postfix_increment_result<Iterator, Value, Reference,
            std::enable_if_t<traits::has_category_v<Iterator, std::input_iterator_tag> &&
                !is_non_proxy_reference_v<Reference, Value>>>
        {
            using type = writable_postfix_increment_proxy<Iterator>;
        };

        template <typename Iterator, typename Value, typename Reference>
        using postfix_increment_result_t =
            typename postfix_increment_result<Iterator, Value, Reference>::type;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
    template <typename Derived, typename T, typename Category, typename Reference,
        typename Distance, typename Pointer>
    PIKA_HOST_DEVICE inline util::detail::postfix_increment_result_t<Derived,
        typename Derived::value_type, typename Derived::reference>
    operator++(iterator_facade<Derived, T, Category, Reference, Distance, Pointer>& i, int)
    {
        using iterator_type = util::detail::postfix_increment_result_t<Derived,
            typename Derived::value_type, typename Derived::reference>;

        iterator_type tmp(*static_cast<Derived*>(&i));
        ++i;
        return tmp;
    }

    namespace detail {
        template <typename Facade1, typename Facade2, typename Return>
        struct enable_operator_interoperable
          : std::enable_if<std::is_convertible_v<Facade1, Facade2> ||
                    std::is_convertible_v<Facade2, Facade1>,
                Return>
        {
        };

        template <typename Facade1, typename Facade2, typename Return, typename Cond>
        struct enable_operator_interoperable_ex
          : std::enable_if<Cond::value &&
                    (std::is_convertible_v<Facade1, Facade2> ||
                        std::is_convertible_v<Facade2, Facade1>),
                Return>
        {
        };
    }    // namespace detail

#define PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD(prefix, op, result_type)                            \
 template <typename Derived1, typename T1, typename Category1, typename Reference1,                \
     typename Distance1, typename Pointer1, typename Derived2, typename T2, typename Category2,    \
     typename Reference2, typename Distance2, typename Pointer2>                                   \
 PIKA_HOST_DEVICE prefix typename pika::util::detail::enable_operator_interoperable<Derived1,      \
     Derived2, result_type>::type                                                                  \
 operator op(iterator_facade<Derived1, T1, Category1, Reference1, Distance1, Pointer1> const& lhs, \
     iterator_facade<Derived2, T2, Category2, Reference2, Distance2, Pointer2> const& rhs) /**/

#define PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD_EX(prefix, op, result_type, cond)                   \
 template <typename Derived1, typename T1, typename Category1, typename Reference1,                \
     typename Distance1, typename Pointer1, typename Derived2, typename T2, typename Category2,    \
     typename Reference2, typename Distance2, typename Pointer2>                                   \
 PIKA_HOST_DEVICE prefix typename pika::util::detail::enable_operator_interoperable_ex<Derived1,   \
     Derived2, result_type,                                                                        \
     cond<typename Derived1::iterator_category, typename Derived2::iterator_category>>::type       \
 operator op(iterator_facade<Derived1, T1, Category1, Reference1, Distance1, Pointer1> const& lhs, \
     iterator_facade<Derived2, T2, Category2, Reference2, Distance2, Pointer2> const& rhs) /**/

    PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD(inline, ==, bool)
    {
        return iterator_core_access::equal(
            static_cast<Derived1 const&>(lhs), static_cast<Derived2 const&>(rhs));
    }

    PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD(inline, !=, bool)
    {
        return !iterator_core_access::equal(
            static_cast<Derived1 const&>(lhs), static_cast<Derived2 const&>(rhs));
    }

    namespace detail {

        template <typename Category1, typename Category2>
        struct enable_random_access_operations
          : std::integral_constant<bool,
                std::is_same_v<Category1, std::random_access_iterator_tag> &&
                    std::is_same_v<Category2, std::random_access_iterator_tag>>
        {
        };
    }    // namespace detail

    PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD_EX(
        inline, <, bool, detail::enable_random_access_operations)
    {
        return 0 < iterator_core_access::distance_to(
                       static_cast<Derived1 const&>(lhs), static_cast<Derived2 const&>(rhs));
    }

    PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD_EX(
        inline, >, bool, detail::enable_random_access_operations)
    {
        return 0 > iterator_core_access::distance_to(
                       static_cast<Derived1 const&>(lhs), static_cast<Derived2 const&>(rhs));
    }

    PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD_EX(
        inline, <=, bool, detail::enable_random_access_operations)
    {
        return 0 <= iterator_core_access::distance_to(
                        static_cast<Derived1 const&>(lhs), static_cast<Derived2 const&>(rhs));
    }

    PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD_EX(
        inline, >=, bool, detail::enable_random_access_operations)
    {
        return 0 >= iterator_core_access::distance_to(
                        static_cast<Derived1 const&>(lhs), static_cast<Derived2 const&>(rhs));
    }

    PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD_EX(inline, -,
        typename std::iterator_traits<Derived2>::difference_type,
        detail::enable_random_access_operations)
    {
        return iterator_core_access::distance_to(
            static_cast<Derived1 const&>(rhs), static_cast<Derived2 const&>(lhs));
    }

#undef PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD_EX
#undef PIKA_UTIL_ITERATOR_FACADE_INTEROP_HEAD

    PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
    template <typename Derived, typename T, typename Category, typename Reference,
        typename Distance, typename Pointer>
    PIKA_HOST_DEVICE inline std::enable_if_t<
        std::is_same_v<typename Derived::iterator_category, std::random_access_iterator_tag>,
        Derived>
    operator+(iterator_facade<Derived, T, Category, Reference, Distance, Pointer> const& it,
        typename Derived::difference_type n)
    {
        Derived tmp(static_cast<Derived const&>(it));
        return tmp += n;
    }

    PIKA_NVCC_PRAGMA_HD_WARNING_DISABLE
    template <typename Derived, typename T, typename Category, typename Reference,
        typename Distance, typename Pointer>
    PIKA_HOST_DEVICE inline std::enable_if_t<
        std::is_same_v<typename Derived::iterator_category, std::random_access_iterator_tag>,
        Derived>
    operator+(typename Derived::difference_type n,
        iterator_facade<Derived, T, Category, Reference, Distance, Pointer> const& it)
    {
        Derived tmp(static_cast<Derived const&>(it));
        return tmp += n;
    }
}    // namespace pika::util
