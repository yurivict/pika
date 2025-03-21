//  Taken from the Boost.Bind library
//
//  bind_test.cpp - monolithic test for bind.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2001 David Abrahams
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(PIKA_MSVC)
# pragma warning(disable : 4786)    // identifier truncated in debug info
# pragma warning(disable : 4710)    // function not inlined
# pragma warning(disable : 4711)    // function selected for automatic inline expansion
# pragma warning(disable : 4514)    // unreferenced inline removed
#endif

#include <pika/functional/bind.hpp>
#include <pika/testing.hpp>

#include <functional>
#include <iostream>

namespace placeholders = std::placeholders;

//

long f_0()
{
    return 17041L;
}

long f_1(long a)
{
    return a;
}

long f_2(long a, long b)
{
    return a + 10 * b;
}

long f_3(long a, long b, long c)
{
    return a + 10 * b + 100 * c;
}

long f_4(long a, long b, long c, long d)
{
    return a + 10 * b + 100 * c + 1000 * d;
}

long f_5(long a, long b, long c, long d, long e)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e;
}

long f_6(long a, long b, long c, long d, long e, long f)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

long f_7(long a, long b, long c, long d, long e, long f, long g)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
}

long f_8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
}

long f_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
{
    return a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h +
        100000000 * i;
}

long global_result;

void fv_0()
{
    global_result = 17041L;
}

void fv_1(long a)
{
    global_result = a;
}

void fv_2(long a, long b)
{
    global_result = a + 10 * b;
}

void fv_3(long a, long b, long c)
{
    global_result = a + 10 * b + 100 * c;
}

void fv_4(long a, long b, long c, long d)
{
    global_result = a + 10 * b + 100 * c + 1000 * d;
}

void fv_5(long a, long b, long c, long d, long e)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e;
}

void fv_6(long a, long b, long c, long d, long e, long f)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f;
}

void fv_7(long a, long b, long c, long d, long e, long f, long g)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g;
}

void fv_8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    global_result =
        a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g + 10000000 * h;
}

void fv_9(long a, long b, long c, long d, long e, long f, long g, long h, long i)
{
    global_result = a + 10 * b + 100 * c + 1000 * d + 10000 * e + 100000 * f + 1000000 * g +
        10000000 * h + 100000000 * i;
}

void function_test()
{
    int const i = 1;

    PIKA_TEST(pika::util::detail::bind(f_0)(i) == 17041L);
    PIKA_TEST(pika::util::detail::bind(f_1, placeholders::_1)(i) == 1L);
    PIKA_TEST(pika::util::detail::bind(f_2, placeholders::_1, 2)(i) == 21L);
    PIKA_TEST(pika::util::detail::bind(f_3, placeholders::_1, 2, 3)(i) == 321L);
    PIKA_TEST(pika::util::detail::bind(f_4, placeholders::_1, 2, 3, 4)(i) == 4321L);
    PIKA_TEST(pika::util::detail::bind(f_5, placeholders::_1, 2, 3, 4, 5)(i) == 54321L);
    PIKA_TEST(pika::util::detail::bind(f_6, placeholders::_1, 2, 3, 4, 5, 6)(i) == 654321L);
    PIKA_TEST(pika::util::detail::bind(f_7, placeholders::_1, 2, 3, 4, 5, 6, 7)(i) == 7654321L);
    PIKA_TEST(pika::util::detail::bind(f_8, placeholders::_1, 2, 3, 4, 5, 6, 7, 8)(i) == 87654321L);
    PIKA_TEST(
        pika::util::detail::bind(f_9, placeholders::_1, 2, 3, 4, 5, 6, 7, 8, 9)(i) == 987654321L);

    PIKA_TEST((pika::util::detail::bind(fv_0)(i), (global_result == 17041L)));
    PIKA_TEST((pika::util::detail::bind(fv_1, placeholders::_1)(i), (global_result == 1L)));
    PIKA_TEST((pika::util::detail::bind(fv_2, placeholders::_1, 2)(i), (global_result == 21L)));
    PIKA_TEST((pika::util::detail::bind(fv_3, placeholders::_1, 2, 3)(i), (global_result == 321L)));
    PIKA_TEST(
        (pika::util::detail::bind(fv_4, placeholders::_1, 2, 3, 4)(i), (global_result == 4321L)));
    PIKA_TEST((pika::util::detail::bind(fv_5, placeholders::_1, 2, 3, 4, 5)(i),
        (global_result == 54321L)));
    PIKA_TEST((pika::util::detail::bind(fv_6, placeholders::_1, 2, 3, 4, 5, 6)(i),
        (global_result == 654321L)));
    PIKA_TEST((pika::util::detail::bind(fv_7, placeholders::_1, 2, 3, 4, 5, 6, 7)(i),
        (global_result == 7654321L)));
    PIKA_TEST((pika::util::detail::bind(fv_8, placeholders::_1, 2, 3, 4, 5, 6, 7, 8)(i),
        (global_result == 87654321L)));
    PIKA_TEST((pika::util::detail::bind(fv_9, placeholders::_1, 2, 3, 4, 5, 6, 7, 8, 9)(i),
        (global_result == 987654321L)));
}

//

struct Y
{
    short operator()(short& r) const
    {
        return ++r;
    }
    int operator()(int a, int b) const
    {
        return a + 10 * b;
    }
    long operator()(long a, long b, long c) const
    {
        return a + 10 * b + 100 * c;
    }
    void operator()(long a, long b, long c, long d) const
    {
        global_result = a + 10 * b + 100 * c + 1000 * d;
    }
};

void function_object_test()
{
    using std::ref;

    short i(6);

    int const k = 3;

    PIKA_TEST(pika::util::detail::bind(Y(), ref(i))() == 7);
    PIKA_TEST(pika::util::detail::bind(Y(), ref(i))() == 8);
    PIKA_TEST(pika::util::detail::bind(Y(), i, placeholders::_1)(k) == 38);
    PIKA_TEST(pika::util::detail::bind(Y(), i, placeholders::_1, 9)(k) == 938);

#if !defined(__MWERKS__) || (__MWERKS__ > 0x2407)
    // Fails for this version of the compiler.

    global_result = 0;
    pika::util::detail::bind(Y(), i, placeholders::_1, 9, 4)(k);
    PIKA_TEST_EQ(global_result, 4938);

#endif
}

void function_object_test2()
{
    using std::ref;

    short i(6);

    int const k = 3;

    PIKA_TEST(pika::util::detail::bind(Y(), ref(i))() == 7);
    PIKA_TEST(pika::util::detail::bind(Y(), ref(i))() == 8);
    PIKA_TEST(pika::util::detail::bind(Y(), i, placeholders::_1)(k) == 38);
    PIKA_TEST(pika::util::detail::bind(Y(), i, placeholders::_1, 9)(k) == 938);

    global_result = 0;
    pika::util::detail::bind(Y(), i, placeholders::_1, 9, 4)(k);
    PIKA_TEST_EQ(global_result, 4938);
}

//

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) &&                                          \
    !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)

struct Z
{
    int operator()(int a, int b) const
    {
        return a + 10 * b;
    }
};

void adaptable_function_object_test()
{
    PIKA_TEST_EQ(pika::util::detail::bind(Z(), 7, 4)(), 47);
}

#endif

//

struct X
{
    mutable unsigned int hash;

    X()
      : hash(0)
    {
    }

    int f0()
    {
        f1(17);
        return 0;
    }
    int g0() const
    {
        g1(17);
        return 0;
    }

    int f1(int a1)
    {
        hash = (hash * 17041 + a1) % 32768;
        return 0;
    }
    int g1(int a1) const
    {
        hash = (hash * 17041 + a1 * 2) % 32768;
        return 0;
    }

    int f2(int a1, int a2)
    {
        f1(a1);
        f1(a2);
        return 0;
    }
    int g2(int a1, int a2) const
    {
        g1(a1);
        g1(a2);
        return 0;
    }

    int f3(int a1, int a2, int a3)
    {
        f2(a1, a2);
        f1(a3);
        return 0;
    }
    int g3(int a1, int a2, int a3) const
    {
        g2(a1, a2);
        g1(a3);
        return 0;
    }

    int f4(int a1, int a2, int a3, int a4)
    {
        f3(a1, a2, a3);
        f1(a4);
        return 0;
    }
    int g4(int a1, int a2, int a3, int a4) const
    {
        g3(a1, a2, a3);
        g1(a4);
        return 0;
    }

    int f5(int a1, int a2, int a3, int a4, int a5)
    {
        f4(a1, a2, a3, a4);
        f1(a5);
        return 0;
    }
    int g5(int a1, int a2, int a3, int a4, int a5) const
    {
        g4(a1, a2, a3, a4);
        g1(a5);
        return 0;
    }

    int f6(int a1, int a2, int a3, int a4, int a5, int a6)
    {
        f5(a1, a2, a3, a4, a5);
        f1(a6);
        return 0;
    }
    int g6(int a1, int a2, int a3, int a4, int a5, int a6) const
    {
        g5(a1, a2, a3, a4, a5);
        g1(a6);
        return 0;
    }

    int f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
    {
        f6(a1, a2, a3, a4, a5, a6);
        f1(a7);
        return 0;
    }
    int g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const
    {
        g6(a1, a2, a3, a4, a5, a6);
        g1(a7);
        return 0;
    }

    int f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
    {
        f7(a1, a2, a3, a4, a5, a6, a7);
        f1(a8);
        return 0;
    }
    int g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const
    {
        g7(a1, a2, a3, a4, a5, a6, a7);
        g1(a8);
        return 0;
    }
};

struct V
{
    mutable unsigned int hash;

    V()
      : hash(0)
    {
    }

    void f0()
    {
        f1(17);
    }
    void g0() const
    {
        g1(17);
    }

    void f1(int a1)
    {
        hash = (hash * 17041 + a1) % 32768;
    }
    void g1(int a1) const
    {
        hash = (hash * 17041 + a1 * 2) % 32768;
    }

    void f2(int a1, int a2)
    {
        f1(a1);
        f1(a2);
    }
    void g2(int a1, int a2) const
    {
        g1(a1);
        g1(a2);
    }

    void f3(int a1, int a2, int a3)
    {
        f2(a1, a2);
        f1(a3);
    }
    void g3(int a1, int a2, int a3) const
    {
        g2(a1, a2);
        g1(a3);
    }

    void f4(int a1, int a2, int a3, int a4)
    {
        f3(a1, a2, a3);
        f1(a4);
    }
    void g4(int a1, int a2, int a3, int a4) const
    {
        g3(a1, a2, a3);
        g1(a4);
    }

    void f5(int a1, int a2, int a3, int a4, int a5)
    {
        f4(a1, a2, a3, a4);
        f1(a5);
    }
    void g5(int a1, int a2, int a3, int a4, int a5) const
    {
        g4(a1, a2, a3, a4);
        g1(a5);
    }

    void f6(int a1, int a2, int a3, int a4, int a5, int a6)
    {
        f5(a1, a2, a3, a4, a5);
        f1(a6);
    }
    void g6(int a1, int a2, int a3, int a4, int a5, int a6) const
    {
        g5(a1, a2, a3, a4, a5);
        g1(a6);
    }

    void f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
    {
        f6(a1, a2, a3, a4, a5, a6);
        f1(a7);
    }
    void g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const
    {
        g6(a1, a2, a3, a4, a5, a6);
        g1(a7);
    }

    void f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
    {
        f7(a1, a2, a3, a4, a5, a6, a7);
        f1(a8);
    }
    void g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const
    {
        g7(a1, a2, a3, a4, a5, a6, a7);
        g1(a8);
    }
};

void member_function_test()
{
    using std::ref;

    X x;

    // 0

    pika::util::detail::bind(&X::f0, &x)();
    pika::util::detail::bind(&X::f0, ref(x))();

    pika::util::detail::bind(&X::g0, &x)();
    pika::util::detail::bind(&X::g0, x)();
    pika::util::detail::bind(&X::g0, ref(x))();

    // 1

    pika::util::detail::bind(&X::f1, &x, 1)();
    pika::util::detail::bind(&X::f1, ref(x), 1)();

    pika::util::detail::bind(&X::g1, &x, 1)();
    pika::util::detail::bind(&X::g1, x, 1)();
    pika::util::detail::bind(&X::g1, ref(x), 1)();

    // 2

    pika::util::detail::bind(&X::f2, &x, 1, 2)();
    pika::util::detail::bind(&X::f2, ref(x), 1, 2)();

    pika::util::detail::bind(&X::g2, &x, 1, 2)();
    pika::util::detail::bind(&X::g2, x, 1, 2)();
    pika::util::detail::bind(&X::g2, ref(x), 1, 2)();

    // 3

    pika::util::detail::bind(&X::f3, &x, 1, 2, 3)();
    pika::util::detail::bind(&X::f3, ref(x), 1, 2, 3)();

    pika::util::detail::bind(&X::g3, &x, 1, 2, 3)();
    pika::util::detail::bind(&X::g3, x, 1, 2, 3)();
    pika::util::detail::bind(&X::g3, ref(x), 1, 2, 3)();

    // 4

    pika::util::detail::bind(&X::f4, &x, 1, 2, 3, 4)();
    pika::util::detail::bind(&X::f4, ref(x), 1, 2, 3, 4)();

    pika::util::detail::bind(&X::g4, &x, 1, 2, 3, 4)();
    pika::util::detail::bind(&X::g4, x, 1, 2, 3, 4)();
    pika::util::detail::bind(&X::g4, ref(x), 1, 2, 3, 4)();

    // 5

    pika::util::detail::bind(&X::f5, &x, 1, 2, 3, 4, 5)();
    pika::util::detail::bind(&X::f5, ref(x), 1, 2, 3, 4, 5)();

    pika::util::detail::bind(&X::g5, &x, 1, 2, 3, 4, 5)();
    pika::util::detail::bind(&X::g5, x, 1, 2, 3, 4, 5)();
    pika::util::detail::bind(&X::g5, ref(x), 1, 2, 3, 4, 5)();

    // 6

    pika::util::detail::bind(&X::f6, &x, 1, 2, 3, 4, 5, 6)();
    pika::util::detail::bind(&X::f6, ref(x), 1, 2, 3, 4, 5, 6)();

    pika::util::detail::bind(&X::g6, &x, 1, 2, 3, 4, 5, 6)();
    pika::util::detail::bind(&X::g6, x, 1, 2, 3, 4, 5, 6)();
    pika::util::detail::bind(&X::g6, ref(x), 1, 2, 3, 4, 5, 6)();

    // 7

    pika::util::detail::bind(&X::f7, &x, 1, 2, 3, 4, 5, 6, 7)();
    pika::util::detail::bind(&X::f7, ref(x), 1, 2, 3, 4, 5, 6, 7)();

    pika::util::detail::bind(&X::g7, &x, 1, 2, 3, 4, 5, 6, 7)();
    pika::util::detail::bind(&X::g7, x, 1, 2, 3, 4, 5, 6, 7)();
    pika::util::detail::bind(&X::g7, ref(x), 1, 2, 3, 4, 5, 6, 7)();

    // 8

    pika::util::detail::bind(&X::f8, &x, 1, 2, 3, 4, 5, 6, 7, 8)();
    pika::util::detail::bind(&X::f8, ref(x), 1, 2, 3, 4, 5, 6, 7, 8)();

    pika::util::detail::bind(&X::g8, &x, 1, 2, 3, 4, 5, 6, 7, 8)();
    pika::util::detail::bind(&X::g8, x, 1, 2, 3, 4, 5, 6, 7, 8)();
    pika::util::detail::bind(&X::g8, ref(x), 1, 2, 3, 4, 5, 6, 7, 8)();

    PIKA_TEST_EQ(x.hash, static_cast<unsigned int>(23558));
}

void member_function_void_test()
{
    using std::ref;

    V v;

    // 0

    pika::util::detail::bind(&V::f0, &v)();
    pika::util::detail::bind(&V::f0, ref(v))();

    pika::util::detail::bind(&V::g0, &v)();
    pika::util::detail::bind(&V::g0, v)();
    pika::util::detail::bind(&V::g0, ref(v))();

    // 1

    pika::util::detail::bind(&V::f1, &v, 1)();
    pika::util::detail::bind(&V::f1, ref(v), 1)();

    pika::util::detail::bind(&V::g1, &v, 1)();
    pika::util::detail::bind(&V::g1, v, 1)();
    pika::util::detail::bind(&V::g1, ref(v), 1)();

    // 2

    pika::util::detail::bind(&V::f2, &v, 1, 2)();
    pika::util::detail::bind(&V::f2, ref(v), 1, 2)();

    pika::util::detail::bind(&V::g2, &v, 1, 2)();
    pika::util::detail::bind(&V::g2, v, 1, 2)();
    pika::util::detail::bind(&V::g2, ref(v), 1, 2)();

    // 3

    pika::util::detail::bind(&V::f3, &v, 1, 2, 3)();
    pika::util::detail::bind(&V::f3, ref(v), 1, 2, 3)();

    pika::util::detail::bind(&V::g3, &v, 1, 2, 3)();
    pika::util::detail::bind(&V::g3, v, 1, 2, 3)();
    pika::util::detail::bind(&V::g3, ref(v), 1, 2, 3)();

    // 4

    pika::util::detail::bind(&V::f4, &v, 1, 2, 3, 4)();
    pika::util::detail::bind(&V::f4, ref(v), 1, 2, 3, 4)();

    pika::util::detail::bind(&V::g4, &v, 1, 2, 3, 4)();
    pika::util::detail::bind(&V::g4, v, 1, 2, 3, 4)();
    pika::util::detail::bind(&V::g4, ref(v), 1, 2, 3, 4)();

    // 5

    pika::util::detail::bind(&V::f5, &v, 1, 2, 3, 4, 5)();
    pika::util::detail::bind(&V::f5, ref(v), 1, 2, 3, 4, 5)();

    pika::util::detail::bind(&V::g5, &v, 1, 2, 3, 4, 5)();
    pika::util::detail::bind(&V::g5, v, 1, 2, 3, 4, 5)();
    pika::util::detail::bind(&V::g5, ref(v), 1, 2, 3, 4, 5)();

    // 6

    pika::util::detail::bind(&V::f6, &v, 1, 2, 3, 4, 5, 6)();
    pika::util::detail::bind(&V::f6, ref(v), 1, 2, 3, 4, 5, 6)();

    pika::util::detail::bind(&V::g6, &v, 1, 2, 3, 4, 5, 6)();
    pika::util::detail::bind(&V::g6, v, 1, 2, 3, 4, 5, 6)();
    pika::util::detail::bind(&V::g6, ref(v), 1, 2, 3, 4, 5, 6)();

    // 7

    pika::util::detail::bind(&V::f7, &v, 1, 2, 3, 4, 5, 6, 7)();
    pika::util::detail::bind(&V::f7, ref(v), 1, 2, 3, 4, 5, 6, 7)();

    pika::util::detail::bind(&V::g7, &v, 1, 2, 3, 4, 5, 6, 7)();
    pika::util::detail::bind(&V::g7, v, 1, 2, 3, 4, 5, 6, 7)();
    pika::util::detail::bind(&V::g7, ref(v), 1, 2, 3, 4, 5, 6, 7)();

    // 8

    pika::util::detail::bind(&V::f8, &v, 1, 2, 3, 4, 5, 6, 7, 8)();
    pika::util::detail::bind(&V::f8, ref(v), 1, 2, 3, 4, 5, 6, 7, 8)();

    pika::util::detail::bind(&V::g8, &v, 1, 2, 3, 4, 5, 6, 7, 8)();
    pika::util::detail::bind(&V::g8, v, 1, 2, 3, 4, 5, 6, 7, 8)();
    pika::util::detail::bind(&V::g8, ref(v), 1, 2, 3, 4, 5, 6, 7, 8)();

    PIKA_TEST_EQ(v.hash, static_cast<unsigned int>(23558));
}

void nested_bind_test()
{
    int const x = 1;
    int const y = 2;

    PIKA_TEST(
        pika::util::detail::bind(f_1, pika::util::detail::bind(f_1, placeholders::_1))(x) == 1L);
    PIKA_TEST(pika::util::detail::bind(f_1,
                  pika::util::detail::bind(f_2, placeholders::_1, placeholders::_2))(x, y) == 21L);
    PIKA_TEST(pika::util::detail::bind(f_2, pika::util::detail::bind(f_1, placeholders::_1),
                  pika::util::detail::bind(f_1, placeholders::_1))(x) == 11L);
    PIKA_TEST(pika::util::detail::bind(f_2, pika::util::detail::bind(f_1, placeholders::_1),
                  pika::util::detail::bind(f_1, placeholders::_2))(x, y) == 21L);
    PIKA_TEST(pika::util::detail::bind(f_1, pika::util::detail::bind(f_0))() == 17041L);

    PIKA_TEST((pika::util::detail::bind(fv_1, pika::util::detail::bind(f_1, placeholders::_1))(x),
        (global_result == 1L)));
    PIKA_TEST((pika::util::detail::bind(
                   fv_1, pika::util::detail::bind(f_2, placeholders::_1, placeholders::_2))(x, y),
        (global_result == 21L)));
    PIKA_TEST((pika::util::detail::bind(fv_2, pika::util::detail::bind(f_1, placeholders::_1),
                   pika::util::detail::bind(f_1, placeholders::_1))(x),
        (global_result == 11L)));
    PIKA_TEST((pika::util::detail::bind(fv_2, pika::util::detail::bind(f_1, placeholders::_1),
                   pika::util::detail::bind(f_1, placeholders::_2))(x, y),
        (global_result == 21L)));
    PIKA_TEST((pika::util::detail::bind(fv_1, pika::util::detail::bind(f_0))(),
        (global_result == 17041L)));
}

int main()
{
    function_test();
    function_object_test();
    function_object_test2();
    adaptable_function_object_test();
    member_function_test();
    member_function_void_test();
    nested_bind_test();

    return 0;
}
