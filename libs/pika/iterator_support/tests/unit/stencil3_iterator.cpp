//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/init.hpp>
#include <pika/modules/iterator_support.hpp>
#include <pika/testing.hpp>

#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace test {
    template <typename Iterator>
    PIKA_FORCEINLINE Iterator previous(Iterator it)
    {
        return --it;
    }

    template <typename Iterator>
    PIKA_FORCEINLINE Iterator next(Iterator it)
    {
        return ++it;
    }

    namespace detail {
        struct stencil_transformer
        {
            template <typename Iterator>
            struct result
            {
                using element_type = typename std::iterator_traits<Iterator>::reference;
                using type = std::tuple<element_type, element_type, element_type>;
            };

            // it will dereference tuple(it-1, it, it+1)
            template <typename Iterator>
            typename result<Iterator>::type operator()(Iterator const& it) const
            {
                using type = typename result<Iterator>::type;
                return type(*test::previous(it), *it, *test::next(it));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator, typename Transformer = detail::stencil_transformer>
    class stencil3_iterator : public pika::util::transform_iterator<Iterator, Transformer>
    {
    private:
        using base_type = pika::util::transform_iterator<Iterator, Transformer>;

    public:
        stencil3_iterator() {}

        explicit stencil3_iterator(Iterator const& it)
          : base_type(it, Transformer())
        {
        }

        stencil3_iterator(Iterator const& it, Transformer const& t)
          : base_type(it, t)
        {
        }
    };

    template <typename Iterator, typename Transformer>
    inline stencil3_iterator<Iterator, Transformer>
    make_stencil3_iterator(Iterator const& it, Transformer const& t)
    {
        return stencil3_iterator<Iterator, Transformer>(it, t);
    }

    template <typename Iterator, typename Transformer>
    inline std::pair<stencil3_iterator<Iterator, Transformer>,
        stencil3_iterator<Iterator, Transformer>>
    make_stencil3_range(Iterator const& begin, Iterator const& end, Transformer const& t)
    {
        return std::make_pair(make_stencil3_iterator(begin, t), make_stencil3_iterator(end, t));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
    inline stencil3_iterator<Iterator> make_stencil3_iterator(Iterator const& it)
    {
        return stencil3_iterator<Iterator>(it);
    }

    template <typename Iterator>
    inline std::pair<stencil3_iterator<Iterator>, stencil3_iterator<Iterator>>
    make_stencil3_range(Iterator const& begin, Iterator const& end)
    {
        return std::make_pair(make_stencil3_iterator(begin), make_stencil3_iterator(end));
    }
}    // namespace test

///////////////////////////////////////////////////////////////////////////////
void test_stencil3_iterator()
{
    std::vector<int> values(10);
    std::iota(std::begin(values), std::end(values), 0);

    auto r = test::make_stencil3_range(values.begin() + 1, values.end() - 1);

    using reference = std::iterator_traits<decltype(r.first)>::reference;

    std::ostringstream str;

    std::for_each(r.first, r.second, [&str](reference val) {
        using std::get;
        str << get<0>(val) << get<1>(val) << get<2>(val) << " ";
    });

    PIKA_TEST_EQ(str.str(), std::string("012 123 234 345 456 567 678 789 "));
}

///////////////////////////////////////////////////////////////////////////////
namespace test {
    template <typename F>
    struct custom_stencil_transformer
    {
        template <typename Iterator>
        struct result
        {
            using element_type = typename std::iterator_traits<Iterator>::reference;
            using value_type = std::invoke_result_t<F, element_type>;

            using type = std::tuple<value_type, element_type, value_type>;
        };

        custom_stencil_transformer(F f)
          : f_(std::move(f))
        {
        }

        // it will dereference tuple(it-1, it, it+1)
        template <typename Iterator>
        typename result<Iterator>::type operator()(Iterator const& it) const
        {
            using type = typename result<Iterator>::type;
            return type(f_(*test::previous(it)), *it, f_(*test::next(it)));
        }

        F f_;
    };

    template <typename F>
    inline custom_stencil_transformer<std::decay_t<F>> make_custom_stencil_transformer(F&& f)
    {
        using transformer_type = custom_stencil_transformer<std::decay_t<F>>;
        return transformer_type(std::forward<F>(f));
    }
}    // namespace test

void test_stencil3_iterator_custom()
{
    std::vector<int> values(10);
    std::iota(std::begin(values), std::end(values), 0);

    auto t = test::make_custom_stencil_transformer([](int i) -> int { return 2 * i; });
    auto r = test::make_stencil3_range(values.begin() + 1, values.end() - 1, t);

    using reference = std::iterator_traits<decltype(r.first)>::reference;

    std::ostringstream str;

    std::for_each(r.first, r.second, [&str](reference val) {
        using std::get;
        str << get<0>(val) << get<1>(val) << get<2>(val) << " ";
    });

    PIKA_TEST_EQ(str.str(), std::string("014 226 438 6410 8512 10614 12716 14818 "));
}

///////////////////////////////////////////////////////////////////////////////
int pika_main()
{
    test_stencil3_iterator();
    test_stencil3_iterator_custom();

    return pika::finalize();
}

int main(int argc, char* argv[])
{
    PIKA_TEST_EQ(pika::init(pika_main, argc, argv), 0);
    return 0;
}
