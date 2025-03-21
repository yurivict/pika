//
//  intrusive_ptr.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/memory/config/defines.hpp>
#include <pika/memory/detail/sp_convertible.hpp>

#include <fmt/format.h>

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <type_traits>

namespace pika::memory {

    //
    //  intrusive_ptr
    //
    //  A smart pointer that uses intrusive reference counting.
    //
    //  Relies on unqualified calls to
    //
    //      void intrusive_ptr_add_ref(T * p);
    //      void intrusive_ptr_release(T * p);
    //
    //          (p != nullptr)
    //
    //  The object is responsible for destroying itself.
    //
    template <typename T>
    class intrusive_ptr
    {
    private:
        using this_type = intrusive_ptr;

    public:
        using element_type = T;

        constexpr intrusive_ptr() noexcept = default;

        intrusive_ptr(T* p, bool add_ref = true)
          : px(p)
        {
            if (px != nullptr && add_ref)
                intrusive_ptr_add_ref(px);
        }

        template <typename U,
            typename Enable = std::enable_if_t<memory::detail::sp_convertible_v<U, T>>>
        intrusive_ptr(intrusive_ptr<U> const& rhs)
          : px(rhs.get())
        {
            if (px != nullptr)
                intrusive_ptr_add_ref(px);
        }

#if defined(PIKA_GCC_VERSION) && PIKA_GCC_VERSION >= 120000
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wuse-after-free"
#endif
        intrusive_ptr(intrusive_ptr const& rhs)
          : px(rhs.px)
        {
#if defined(PIKA_GCC_VERSION) && PIKA_GCC_VERSION >= 120000
# pragma GCC diagnostic pop
#endif
            if (px != nullptr)
                intrusive_ptr_add_ref(px);
        }

        ~intrusive_ptr()
        {
            if (px != nullptr)
                intrusive_ptr_release(px);
        }

        template <typename U>
        intrusive_ptr& operator=(intrusive_ptr<U> const& rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        // Move support
        constexpr intrusive_ptr(intrusive_ptr&& rhs) noexcept
          : px(rhs.px)
        {
            rhs.px = nullptr;
        }

        intrusive_ptr& operator=(intrusive_ptr&& rhs) noexcept
        {
            this_type(static_cast<intrusive_ptr&&>(rhs)).swap(*this);
            return *this;
        }

        template <typename U>
        friend class intrusive_ptr;

        template <typename U,
            typename Enable = std::enable_if_t<memory::detail::sp_convertible_v<U, T>>>
        constexpr intrusive_ptr(intrusive_ptr<U>&& rhs) noexcept
          : px(rhs.px)
        {
            rhs.px = nullptr;
        }

        template <typename U>
        intrusive_ptr& operator=(intrusive_ptr<U>&& rhs) noexcept
        {
            this_type(static_cast<intrusive_ptr<U>&&>(rhs)).swap(*this);
            return *this;
        }

        // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
        intrusive_ptr& operator=(intrusive_ptr const& rhs) noexcept
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        intrusive_ptr& operator=(T* rhs) noexcept
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        void reset() noexcept
        {
            this_type().swap(*this);
        }

        void reset(T* rhs) noexcept
        {
            this_type(rhs).swap(*this);
        }

        void reset(T* rhs, bool add_ref) noexcept
        {
            this_type(rhs, add_ref).swap(*this);
        }

        constexpr T* get() const noexcept
        {
            return px;
        }

        constexpr T* detach() noexcept
        {
            T* ret = px;
            px = nullptr;
            return ret;
        }

        T& operator*() const noexcept
        {
            PIKA_ASSERT(px != nullptr);
            return *px;
        }

        T* operator->() const noexcept
        {
            PIKA_ASSERT(px != nullptr);
            return px;
        }

        explicit constexpr operator bool() const noexcept
        {
            return px != nullptr;
        }

        constexpr void swap(intrusive_ptr& rhs) noexcept
        {
            T* tmp = px;
            px = rhs.px;
            rhs.px = tmp;
        }

    private:
        T* px = nullptr;
    };

    template <typename T, typename U>
    inline constexpr bool operator==(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() == b.get();
    }

    template <typename T, typename U>
    inline constexpr bool operator!=(intrusive_ptr<T> const& a, intrusive_ptr<U> const& b) noexcept
    {
        return a.get() != b.get();
    }

    template <typename T, typename U>
    inline constexpr bool operator==(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() == b;
    }

    template <typename T, typename U>
    inline constexpr bool operator!=(intrusive_ptr<T> const& a, U* b) noexcept
    {
        return a.get() != b;
    }

    template <typename T, typename U>
    inline constexpr bool operator==(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a == b.get();
    }

    template <typename T, typename U>
    inline constexpr bool operator!=(T* a, intrusive_ptr<U> const& b) noexcept
    {
        return a != b.get();
    }

    template <typename T>
    inline constexpr bool operator==(intrusive_ptr<T> const& p, std::nullptr_t) noexcept
    {
        return p.get() == nullptr;
    }

    template <typename T>
    inline constexpr bool operator==(std::nullptr_t, intrusive_ptr<T> const& p) noexcept
    {
        return p.get() == nullptr;
    }

    template <typename T>
    inline constexpr bool operator!=(intrusive_ptr<T> const& p, std::nullptr_t) noexcept
    {
        return p.get() != nullptr;
    }

    template <typename T>
    inline constexpr bool operator!=(std::nullptr_t, intrusive_ptr<T> const& p) noexcept
    {
        return p.get() != nullptr;
    }

    template <typename T>
    inline constexpr bool operator<(intrusive_ptr<T> const& a, intrusive_ptr<T> const& b) noexcept
    {
        return std::less<T*>{}(a.get(), b.get());
    }

    template <typename T>
    void swap(intrusive_ptr<T>& lhs, intrusive_ptr<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    // mem_fn support
    template <typename T>
    constexpr T* get_pointer(intrusive_ptr<T> const& p) noexcept
    {
        return p.get();
    }

    // pointer casts
    template <typename T, typename U>
    constexpr intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const& p)
    {
        return static_cast<T*>(p.get());
    }

    template <typename T, typename U>
    constexpr intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const& p)
    {
        return const_cast<T*>(p.get());
    }

    template <typename T, typename U>
    intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const& p)
    {
        return dynamic_cast<T*>(p.get());
    }

    template <typename T, typename U>
    constexpr intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U>&& p) noexcept
    {
        return intrusive_ptr<T>(static_cast<T*>(p.detach()), false);
    }

    template <typename T, typename U>
    constexpr intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U>&& p) noexcept
    {
        return intrusive_ptr<T>(const_cast<T*>(p.detach()), false);
    }

    template <typename T, typename U>
    intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U>&& p) noexcept
    {
        T* p2 = dynamic_cast<T*>(p.get());

        intrusive_ptr<T> r(p2, false);

        if (p2)
            p.detach();

        return r;
    }

    // operator<<
    template <typename Y>
    std::ostream& operator<<(std::ostream& os, intrusive_ptr<Y> const& p)
    {
        os << p.get();
        return os;
    }
}    // namespace pika::memory

namespace pika {

    // hoist intrusive_ptr and friends into this namespace
    using pika::memory::intrusive_ptr;

    using pika::memory::get_pointer;

    using pika::memory::const_pointer_cast;
    using pika::memory::dynamic_pointer_cast;
    using pika::memory::static_pointer_cast;
}    // namespace pika

template <typename T>
struct fmt::formatter<pika::memory::intrusive_ptr<T>> : fmt::formatter<void*>
{
    template <typename FormatContext>
    auto format(pika::memory::intrusive_ptr<T> p, FormatContext& ctx)
    {
        return fmt::formatter<void*>::format(static_cast<void*>(p.get()), ctx);
    }
};

namespace std {

    // support hashing
    template <typename T>
    struct hash<pika::memory::intrusive_ptr<T>>
    {
        using result_type = std::size_t;

        result_type operator()(pika::memory::intrusive_ptr<T> const& p) const noexcept
        {
            return hash<T*>{}(p.get());
        }
    };
}    // namespace std
