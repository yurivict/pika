//  Taken from the Boost.Function library

//  Copyright Douglas Gregor 2001-2003.
//  Copyright 2013 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <pika/functional/function.hpp>
#include <pika/testing.hpp>

#include <cstddef>

static int alloc_count = 0;
static int dealloc_count = 0;

template <typename T>
struct counting_allocator : public std::allocator<T>
{
    template <typename U>
    struct rebind
    {
        using other = counting_allocator<U>;
    };

    counting_allocator() {}

    template <typename U>
    counting_allocator(counting_allocator<U>)
    {
    }

    T* allocate(std::size_t n)
    {
        alloc_count++;
        return std::allocator<T>::allocate(n);
    }

    void deallocate(T* p, std::size_t n)
    {
        dealloc_count++;
        std::allocator<T>::deallocate(p, n);
    }
};

struct enable_small_object_optimization
{
};

struct disable_small_object_optimization
{
    int unused_state_data[32];
};

template <typename base>
struct plus_int : base
{
    int operator()(int x, int y) const
    {
        return x + y;
    }
};

static int do_minus(int x, int y)
{
    return x - y;
}

template <typename base>
struct DoNothing : base
{
    void operator()() const {}
};

static void do_nothing() {}

int main(int, char*[])
{
    pika::util::detail::function<int(int, int)> f;
    f.assign(plus_int<disable_small_object_optimization>(), counting_allocator<int>());
    f.clear();
    PIKA_CHECK(alloc_count == 1);
    PIKA_CHECK(dealloc_count == 1);
    alloc_count = 0;
    dealloc_count = 0;
    f.assign(plus_int<enable_small_object_optimization>(), counting_allocator<int>());
    f.clear();
    PIKA_CHECK(alloc_count == 0);
    PIKA_CHECK(dealloc_count == 0);
    f.assign(plus_int<disable_small_object_optimization>(), std::allocator<int>());
    f.clear();
    f.assign(plus_int<enable_small_object_optimization>(), std::allocator<int>());
    f.clear();

    alloc_count = 0;
    dealloc_count = 0;
    f.assign(&do_minus, counting_allocator<int>());
    f.clear();
    PIKA_CHECK(alloc_count == 0);
    PIKA_CHECK(dealloc_count == 0);
    f.assign(&do_minus, std::allocator<int>());
    f.clear();

    pika::util::detail::function<void()> fv;
    alloc_count = 0;
    dealloc_count = 0;
    fv.assign(DoNothing<disable_small_object_optimization>(), counting_allocator<int>());
    fv.clear();
    PIKA_CHECK(alloc_count == 1);
    PIKA_CHECK(dealloc_count == 1);
    alloc_count = 0;
    dealloc_count = 0;
    fv.assign(DoNothing<enable_small_object_optimization>(), counting_allocator<int>());
    fv.clear();
    PIKA_CHECK(alloc_count == 0);
    PIKA_CHECK(dealloc_count == 0);
    fv.assign(DoNothing<disable_small_object_optimization>(), std::allocator<int>());
    fv.clear();
    fv.assign(DoNothing<enable_small_object_optimization>(), std::allocator<int>());
    fv.clear();

    alloc_count = 0;
    dealloc_count = 0;
    fv.assign(&do_nothing, counting_allocator<int>());
    fv.clear();
    PIKA_CHECK(alloc_count == 0);
    PIKA_CHECK(dealloc_count == 0);
    fv.assign(&do_nothing, std::allocator<int>());
    fv.clear();

    pika::util::detail::function<void()> fv2;
    fv.assign(&do_nothing, std::allocator<int>());
    fv2.assign(fv, std::allocator<int>());

    return 0;
}
