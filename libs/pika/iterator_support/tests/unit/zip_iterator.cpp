// (C) Copyright Dave Abrahams and Thomas Becker 2003.
//
//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/modules/iterator_support.hpp>
#include <pika/testing.hpp>

#include <cstddef>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// Tests for https://svn.boost.org/trac/boost/ticket/1517
int to_value(std::list<int>::const_iterator v)
{
    return *v;
}

void category_test()
{
    std::list<int> rng1;
    std::string rng2;

    pika::util::make_zip_iterator(std::make_tuple(
        // BidirectionalInput
        pika::util::make_transform_iterator(rng1.begin(), &to_value),
        rng2.begin()    // RandomAccess
        ));
}
//

/// Deduces to the result of tuple_cat when it's invoked with the given
/// parameters Ts.
template <typename... Ts>
using tuple_cat_result_of_t = decltype(std::tuple_cat(std::declval<Ts>()...));

int main(void)
{
    category_test();

    //     size_t num_successful_tests = 0;
    //     size_t num_failed_tests = 0;

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator construction and dereferencing
    //
    /////////////////////////////////////////////////////////////////////////////

    std::vector<double> vect1(3);
    vect1[0] = 42.;
    vect1[1] = 43.;
    vect1[2] = 44.;

    std::set<int> intset;
    intset.insert(52);
    intset.insert(53);
    intset.insert(54);
    //

    using zit_mixed =
        pika::util::zip_iterator<std::set<int>::iterator, std::vector<double>::iterator>;

    zit_mixed zip_it_mixed = zit_mixed(std::make_tuple(intset.begin(), vect1.begin()));

    std::tuple<int, double> val_tuple(*zip_it_mixed);

    std::tuple<const int&, double&> ref_tuple(*zip_it_mixed);

    double dblOldVal = std::get<1>(ref_tuple);
    std::get<1>(ref_tuple) -= 41.;

    PIKA_TEST(52 == std::get<0>(val_tuple) && 42. == std::get<1>(val_tuple) &&
        52 == std::get<0>(ref_tuple) && 1. == std::get<1>(ref_tuple) && 1. == *vect1.begin());

    // Undo change to vect1
    std::get<1>(ref_tuple) = dblOldVal;

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator with 12 components
    //
    /////////////////////////////////////////////////////////////////////////////

    // Declare 12 containers
    //
    std::list<int> li1;
    li1.push_back(1);
    std::set<int> se1;
    se1.insert(2);
    std::vector<int> ve1;
    ve1.push_back(3);
    //
    std::list<int> li2;
    li2.push_back(4);
    std::set<int> se2;
    se2.insert(5);
    std::vector<int> ve2;
    ve2.push_back(6);
    //
    std::list<int> li3;
    li3.push_back(7);
    std::set<int> se3;
    se3.insert(8);
    std::vector<int> ve3;
    ve3.push_back(9);
    //
    std::list<int> li4;
    li4.push_back(10);
    std::set<int> se4;
    se4.insert(11);
    std::vector<int> ve4;
    ve4.push_back(12);

    // typedefs for cons lists of iterators.
    using cons_11_its_type = tuple_cat_result_of_t<std::tuple<std::set<int>::iterator>,
        std::tuple<std::vector<int>::iterator, std::list<int>::iterator, std::set<int>::iterator,
            std::vector<int>::iterator, std::list<int>::iterator, std::set<int>::iterator,
            std::vector<int>::iterator, std::list<int>::iterator, std::set<int>::iterator,
            std::vector<int>::const_iterator>>;
    //
    using cons_12_its_type =
        tuple_cat_result_of_t<std::tuple<std::list<int>::const_iterator>, cons_11_its_type>;

    // typedefs for cons lists for dereferencing the zip iterator
    // made from the cons list above.
    using cons_11_refs_type = tuple_cat_result_of_t<std::tuple<const int&>,
        std::tuple<int&, int&, const int&, int&, int&, const int&, int&, int&, const int&,
            const int&>>;
    //
    using cons_12_refs_type = tuple_cat_result_of_t<std::tuple<const int&>, cons_11_refs_type>;

    // typedef for zip iterator with 12 elements
    using zip_it_12_type = pika::util::zip_iterator<cons_12_its_type>;

    // Declare a 12-element zip iterator.
    zip_it_12_type zip_it_12(li1.begin(), se1.begin(), ve1.begin(), li2.begin(), se2.begin(),
        ve2.begin(), li3.begin(), se3.begin(), ve3.begin(), li4.begin(), se4.begin(), ve4.begin());

    // Dereference, mess with the result a little.
    cons_12_refs_type zip_it_12_dereferenced(*zip_it_12);
    std::get<9>(zip_it_12_dereferenced) = 42;

    // Make a copy and move it a little to force some instantiations.
    zip_it_12_type zip_it_12_copy(zip_it_12);
    ++zip_it_12_copy;

    PIKA_TEST(std::get<11>(zip_it_12.get_iterator_tuple()) == ve4.begin() &&
        std::get<11>(zip_it_12_copy.get_iterator_tuple()) == ve4.end() &&
        1 == std::get<0>(zip_it_12_dereferenced) && 12 == std::get<11>(zip_it_12_dereferenced) &&
        42 == *(li4.begin()));

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator incrementing and dereferencing
    //
    /////////////////////////////////////////////////////////////////////////////

    std::vector<double> vect2(3);
    vect2[0] = 2.2;
    vect2[1] = 3.3;
    vect2[2] = 4.4;

    pika::util::zip_iterator<
        std::tuple<std::vector<double>::const_iterator, std::vector<double>::const_iterator>>
        zip_it_begin(std::make_tuple(vect1.begin(), vect2.begin()));

    pika::util::zip_iterator<
        std::tuple<std::vector<double>::const_iterator, std::vector<double>::const_iterator>>
        zip_it_run(std::make_tuple(vect1.begin(), vect2.begin()));

    pika::util::zip_iterator<
        std::tuple<std::vector<double>::const_iterator, std::vector<double>::const_iterator>>
        zip_it_end(std::make_tuple(vect1.end(), vect2.end()));

    PIKA_TEST(zip_it_run == zip_it_begin && 42. == std::get<0>(*zip_it_run) &&
        2.2 == std::get<1>(*zip_it_run) && 43. == std::get<0>(*(++zip_it_run)) &&
        3.3 == std::get<1>(*zip_it_run) && 44. == std::get<0>(*(++zip_it_run)) &&
        4.4 == std::get<1>(*zip_it_run) && zip_it_end == ++zip_it_run);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator decrementing and dereferencing
    //
    /////////////////////////////////////////////////////////////////////////////

    PIKA_TEST(zip_it_run == zip_it_end && zip_it_end == zip_it_run-- &&
        44. == std::get<0>(*zip_it_run) && 4.4 == std::get<1>(*zip_it_run) &&
        43. == std::get<0>(*(--zip_it_run)) && 3.3 == std::get<1>(*zip_it_run) &&
        42. == std::get<0>(*(--zip_it_run)) && 2.2 == std::get<1>(*zip_it_run) &&
        zip_it_begin == zip_it_run);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator copy construction and equality
    //
    /////////////////////////////////////////////////////////////////////////////

    pika::util::zip_iterator<
        std::tuple<std::vector<double>::const_iterator, std::vector<double>::const_iterator>>
        zip_it_run_copy(zip_it_run);

    PIKA_TEST(zip_it_run == zip_it_run && zip_it_run == zip_it_run_copy);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator inequality
    //
    /////////////////////////////////////////////////////////////////////////////

    PIKA_TEST(!(zip_it_run != zip_it_run_copy) && zip_it_run != ++zip_it_run_copy);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator less than
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run_copy == zip_it_run + 1
    //
    PIKA_TEST(zip_it_run < zip_it_run_copy && !(zip_it_run < --zip_it_run_copy) &&
        zip_it_run == zip_it_run_copy);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator less than or equal
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run_copy == zip_it_run
    //
    ++zip_it_run;
    zip_it_run_copy += 2;

    PIKA_TEST(zip_it_run <= zip_it_run_copy && zip_it_run <= --zip_it_run_copy &&
        !(zip_it_run <= --zip_it_run_copy) && zip_it_run <= zip_it_run);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator greater than
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run_copy == zip_it_run - 1
    //
    PIKA_TEST(zip_it_run > zip_it_run_copy && !(zip_it_run > ++zip_it_run_copy) &&
        zip_it_run == zip_it_run_copy);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator greater than or equal
    //
    /////////////////////////////////////////////////////////////////////////////

    ++zip_it_run;

    // Note: zip_it_run == zip_it_run_copy + 1
    //
    PIKA_TEST(zip_it_run >= zip_it_run_copy && --zip_it_run >= zip_it_run_copy &&
        !(zip_it_run >= ++zip_it_run_copy));

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator + int
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run == zip_it_run_copy - 1
    //
    zip_it_run = zip_it_run + 2;
    ++zip_it_run_copy;

    PIKA_TEST(zip_it_run == zip_it_run_copy && zip_it_run == zip_it_begin + 3);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator - int
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run == zip_it_run_copy, and both are at end position
    //
    zip_it_run = zip_it_run - 2;
    --zip_it_run_copy;
    --zip_it_run_copy;

    PIKA_TEST(zip_it_run == zip_it_run_copy && (zip_it_run - 1) == zip_it_begin);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator +=
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run == zip_it_run_copy, and both are at begin + 1
    //
    zip_it_run += 2;
    PIKA_TEST(zip_it_run == zip_it_begin + 3);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator -=
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run is at end position, zip_it_run_copy is at
    // begin plus one.
    //
    zip_it_run -= 2;
    PIKA_TEST(zip_it_run == zip_it_run_copy);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator getting member iterators
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run and zip_it_run_copy are both at
    // begin plus one.
    //
    PIKA_TEST(std::get<0>(zip_it_run.get_iterator_tuple()) == vect1.begin() + 1 &&
        std::get<1>(zip_it_run.get_iterator_tuple()) == vect2.begin() + 1);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Making zip iterators
    //
    /////////////////////////////////////////////////////////////////////////////

    std::vector<std::tuple<double, double>> vect_of_tuples(3);

    std::copy(pika::util::make_zip_iterator(std::make_tuple(vect1.begin(), vect2.begin())),
        pika::util::make_zip_iterator(std::make_tuple(vect1.end(), vect2.end())),
        vect_of_tuples.begin());

    PIKA_TEST(42. == std::get<0>(*vect_of_tuples.begin()) &&
        2.2 == std::get<1>(*vect_of_tuples.begin()) &&
        43. == std::get<0>(*(vect_of_tuples.begin() + 1)) &&
        3.3 == std::get<1>(*(vect_of_tuples.begin() + 1)) &&
        44. == std::get<0>(*(vect_of_tuples.begin() + 2)) &&
        4.4 == std::get<1>(*(vect_of_tuples.begin() + 2)));

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator non-const --> const conversion
    //
    /////////////////////////////////////////////////////////////////////////////

    pika::util::zip_iterator<
        std::tuple<std::set<int>::const_iterator, std::vector<double>::const_iterator>>
        zip_it_const(std::make_tuple(intset.begin(), vect2.begin()));
    //
    pika::util::zip_iterator<
        std::tuple<std::set<int>::iterator, std::vector<double>::const_iterator>>
        zip_it_half_const(std::make_tuple(intset.begin(), vect2.begin()));
    //
    pika::util::zip_iterator<std::tuple<std::set<int>::iterator, std::vector<double>::iterator>>
        zip_it_non_const(std::make_tuple(intset.begin(), vect2.begin()));

    zip_it_half_const = ++zip_it_non_const;
    zip_it_const = zip_it_half_const;
    ++zip_it_const;

    // Error: can't convert from const to non-const
    //  zip_it_non_const = ++zip_it_const;

    PIKA_TEST(54 == std::get<0>(*zip_it_const) && 4.4 == std::get<1>(*zip_it_const) &&
        53 == std::get<0>(*zip_it_half_const) && 3.3 == std::get<1>(*zip_it_half_const));

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator categories
    //
    /////////////////////////////////////////////////////////////////////////////

    // The big iterator of the previous test has vector, list, and set iterators.
    // Therefore, it must be bidirectional, but not random access.
    bool bBigItIsBidirectionalIterator =
        std::is_convertible<pika::util::zip_iterator_category<zip_it_12_type>::type,
            std::bidirectional_iterator_tag>::value;

    bool bBigItIsRandomAccessIterator =
        std::is_convertible<pika::util::zip_iterator_category<zip_it_12_type>::type,
            std::random_access_iterator_tag>::value;

    // A combining iterator with all vector iterators must have random access
    // traversal.
    //
    using all_vects_type = pika::util::zip_iterator<
        std::tuple<std::vector<double>::const_iterator, std::vector<double>::const_iterator>>;

    bool bAllVectsIsRandomAccessIterator =
        std::is_convertible<pika::util::zip_iterator_category<all_vects_type>::type,
            std::random_access_iterator_tag>::value;

    // The big test.
    PIKA_TEST(bBigItIsBidirectionalIterator && !bBigItIsRandomAccessIterator &&
        bAllVectsIsRandomAccessIterator);

    return 0;
}
