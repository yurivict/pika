//  Copyright (c) 2007-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/iterator_support/iterator_adaptor.hpp>
#include <pika/type_support/lazy_conditional.hpp>
#include <pika/type_support/type_identity.hpp>

#include <iterator>
#include <type_traits>

namespace pika::util {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Transformer, typename Reference = void,
        typename Value = void, typename Category = void, typename Difference = void>
    class transform_iterator;

    namespace detail {

        template <typename Iterator, typename Transformer, typename Reference, typename Value,
            typename Category, typename Difference>
        struct transform_iterator_base
        {
            // the following type calculations use lazy_conditional to avoid
            // premature instantiations
            using reference_type =
                ::pika::detail::lazy_conditional_t<std::is_void<Reference>::value,
                    std::invoke_result<Transformer, Iterator>,
                    ::pika::detail::type_identity<Reference>>;

            using value_type = ::pika::detail::lazy_conditional_t<std::is_void<Value>::value,
                std::remove_reference<reference_type>, ::pika::detail::type_identity<Value>>;

            using iterator_category =
                ::pika::detail::lazy_conditional_t<std::is_void<Category>::value,
                    category_iterator_traits_helper<Iterator>,
                    ::pika::detail::type_identity<Category>>;

            using difference_type =
                ::pika::detail::lazy_conditional_t<std::is_void<Difference>::value,
                    difference_type_iterator_traits_helper<Iterator>,
                    ::pika::detail::type_identity<Difference>>;

            using type = pika::util::iterator_adaptor<
                transform_iterator<Iterator, Transformer, Reference, Value, Category, Difference>,
                Iterator, value_type, iterator_category, reference_type, difference_type>;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // The main difference to boost::transform_iterator is that the transformer
    // function will be invoked with the iterator, not with the result of
    // dereferencing the base iterator.
    template <typename Iterator, typename Transformer, typename Reference, typename Value,
        typename Category, typename Difference>
    class transform_iterator
      : public detail::transform_iterator_base<Iterator, Transformer, Reference, Value, Category,
            Difference>::type
    {
    private:
        using base_type = typename detail::transform_iterator_base<Iterator, Transformer, Reference,
            Value, Category, Difference>::type;

    public:
        transform_iterator() {}

        explicit transform_iterator(Iterator const& it)
          : base_type(it)
        {
        }
        transform_iterator(Iterator const& it, Transformer const& f)
          : base_type(it)
          , transformer_(f)
        {
        }

        template <typename OtherIterator, typename OtherTransformer, typename OtherReference,
            typename OtherValue, typename OtherCategory, typename OtherDifference>
        transform_iterator(transform_iterator<OtherIterator, OtherTransformer, OtherReference,
                               OtherValue, OtherCategory, OtherDifference> const& t,
            typename std::enable_if<std::is_convertible<OtherIterator, Iterator>::value &&
                std::is_convertible<OtherTransformer, Transformer>::value &&
                std::is_convertible<OtherCategory, Category>::value &&
                std::is_convertible<OtherDifference, Difference>::value>::type* = nullptr)
          : base_type(t.base())
          , transformer_(t.transformer())
        {
        }

        Transformer const& transformer() const
        {
            return transformer_;
        }

    private:
        friend class pika::util::iterator_core_access;

        typename base_type::reference dereference() const
        {
            return transformer_(this->base());
        }

        Transformer transformer_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Transformer, typename Iterator>
    inline transform_iterator<Iterator, Transformer>
    make_transform_iterator(Iterator const& it, Transformer const& f)
    {
        return transform_iterator<Iterator, Transformer>(it, f);
    }

    template <typename Transformer, typename Iterator>
    inline transform_iterator<Iterator, Transformer> make_transform_iterator(Iterator const& it)
    {
        return transform_iterator<Iterator, Transformer>(it, Transformer());
    }
}    // namespace pika::util
