//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (C) 2011 Vicente J. Botet Escriba
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

struct test_alloc_base
{
    static int count;
    static int throw_after;
};

int test_alloc_base::count = 0;
int test_alloc_base::throw_after = INT_MAX;

template <typename T>
class test_allocator : public test_alloc_base
{
    int data_;

    template <typename U>
    friend class test_allocator;

public:
    using size_type = std::size_t;
    using difference_type = std::int64_t;
    using value_type = T;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = typename std::add_lvalue_reference<value_type>::type;
    using const_reference = typename std::add_lvalue_reference<const value_type>::type;

    template <typename U>
    struct rebind
    {
        using other = test_allocator<U>;
    };

    test_allocator() noexcept
      : data_(-1)
    {
    }

    explicit test_allocator(int i) noexcept
      : data_(i)
    {
    }

    test_allocator(test_allocator const& a) noexcept
      : data_(a.data_)
    {
    }

    template <typename U>
    test_allocator(test_allocator<U> const& a) noexcept
      : data_(a.data_)
    {
    }

    ~test_allocator() noexcept
    {
        data_ = 0;
    }

    pointer address(reference x) const
    {
        return &x;
    }
    const_pointer address(const_reference x) const
    {
        return &x;
    }

    pointer allocate(size_type n, const void* = nullptr)
    {
        if (count >= throw_after)
            throw std::bad_alloc();
        ++count;
        return static_cast<pointer>(std::malloc(n * sizeof(T)));
    }

    void deallocate(pointer p, size_type)
    {
        --count;
        std::free(p);
    }

    size_type max_size() const noexcept
    {
        return UINT_MAX / sizeof(T);
    }

    template <typename U, typename... Ts>
    void construct(U* p, Ts&&... ts)
    {
        ::new ((void*) p) T(std::forward<Ts>(ts)...);
    }

    void destroy(pointer p)
    {
        p->~T();
    }

    friend bool operator==(test_allocator const& x, test_allocator const& y)
    {
        return x.data_ == y.data_;
    }
    friend bool operator!=(test_allocator const& x, test_allocator const& y)
    {
        return !(x == y);
    }
};

template <>
class test_allocator<void> : public test_alloc_base
{
    int data_;

    template <typename U>
    friend class test_allocator;

public:
    using size_type = std::size_t;
    using difference_type = std::int64_t;
    using value_type = void;
    using pointer = value_type*;
    using const_pointer = value_type const*;

    template <typename U>
    struct rebind
    {
        using other = test_allocator<U>;
    };

    test_allocator() noexcept
      : data_(-1)
    {
    }

    explicit test_allocator(int i) noexcept
      : data_(i)
    {
    }

    test_allocator(test_allocator const& a) noexcept
      : data_(a.data_)
    {
    }

    template <typename U>
    test_allocator(test_allocator<U> const& a) noexcept
      : data_(a.data_)
    {
    }

    ~test_allocator() noexcept
    {
        data_ = 0;
    }

    friend bool operator==(test_allocator const& x, test_allocator const& y)
    {
        return x.data_ == y.data_;
    }
    friend bool operator!=(test_allocator const& x, test_allocator const& y)
    {
        return !(x == y);
    }
};
