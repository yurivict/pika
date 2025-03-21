//  Copyright (c) 2017 Agustin Berge
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/errors/error_code.hpp>
#include <pika/errors/exception_info.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

#if defined(PIKA_WINDOWS)
# include <excpt.h>
# undef exception_info
#endif

namespace pika {
    ///////////////////////////////////////////////////////////////////////////
    template <typename Tag, typename Type>
    struct error_info
    {
        using tag = Tag;
        using type = Type;

        explicit error_info(Type const& value)
          : _value(value)
        {
        }

        explicit error_info(Type&& value)
          : _value(PIKA_MOVE(value))
        {
        }

        Type _value;
    };

#define PIKA_DEFINE_ERROR_INFO(NAME, TYPE)                                                         \
 struct NAME : ::pika::error_info<NAME, TYPE>                                                      \
 {                                                                                                 \
  explicit NAME(TYPE const& value)                                                                 \
    : error_info(value)                                                                            \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  explicit NAME(TYPE&& value)                                                                      \
    : error_info(PIKA_FORWARD(TYPE, value))                                                        \
  {                                                                                                \
  }                                                                                                \
 } /**/

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        class exception_info_node_base
        {
        public:
            virtual ~exception_info_node_base() = default;
            virtual void const* lookup(std::type_info const& tag) const noexcept = 0;

            std::shared_ptr<exception_info_node_base> next;
        };

        template <typename... Ts>
        class exception_info_node
          : public exception_info_node_base
          , Ts...
        {
        public:
            template <typename... ErrorInfo>
            explicit exception_info_node(ErrorInfo&&... tagged_values)
              : Ts(tagged_values)...
            {
            }

            void const* lookup(std::type_info const& tag) const noexcept override
            {
                using entry_type = std::pair<std::type_info const&, void const*>;
                entry_type const entries[] = {{typeid(typename Ts::tag),
                    std::addressof(static_cast<Ts const*>(this)->_value)}...};

                for (auto const& entry : entries)
                {
                    if (entry.first == tag)
                        return entry.second;
                }

                return next ? next->lookup(tag) : nullptr;
            }

            using exception_info_node_base::next;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    class exception_info
    {
        using node_ptr = std::shared_ptr<detail::exception_info_node_base>;

    public:
        exception_info() noexcept
          : _data(nullptr)
        {
        }

        exception_info(exception_info const& other) noexcept = default;
        exception_info(exception_info&& other) noexcept = default;

        exception_info& operator=(exception_info const& other) noexcept = default;
        exception_info& operator=(exception_info&& other) noexcept = default;

        virtual ~exception_info() = default;

        template <typename... ErrorInfo>
        exception_info& set(ErrorInfo&&... tagged_values)
        {
            using node_type = detail::exception_info_node<ErrorInfo...>;

            node_ptr node = std::make_shared<node_type>(PIKA_FORWARD(ErrorInfo, tagged_values)...);
            node->next = PIKA_MOVE(_data);
            _data = PIKA_MOVE(node);
            return *this;
        }

        template <typename Tag>
        typename Tag::type const* get() const noexcept
        {
            auto const* data = _data.get();
            return static_cast<typename Tag::type const*>(
                data ? data->lookup(typeid(typename Tag::tag)) : nullptr);
        }

    private:
        node_ptr _data;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        struct exception_with_info_base : public exception_info
        {
            exception_with_info_base(std::type_info const& type, exception_info xi)
              : exception_info(PIKA_MOVE(xi))
              , type(type)
            {
            }

            std::type_info const& type;
        };

        template <typename E>
        struct exception_with_info
          : public E
          , public exception_with_info_base
        {
            explicit exception_with_info(E const& e, exception_info xi)
              : E(e)
              , exception_with_info_base(typeid(E), PIKA_MOVE(xi))
            {
            }

            explicit exception_with_info(E&& e, exception_info xi)
              : E(PIKA_MOVE(e))
              , exception_with_info_base(typeid(E), PIKA_MOVE(xi))
            {
            }
        };
    }    // namespace detail

    template <typename E>
    [[noreturn]] void throw_with_info(E&& e, exception_info&& xi = exception_info())
    {
        using ED = std::decay_t<E>;
        static_assert(
            std::is_class<ED>::value && !std::is_final<ED>::value, "E shall be a valid base class");
        static_assert(
            !std::is_base_of<exception_info, ED>::value, "E shall not derive from exception_info");

        throw detail::exception_with_info<ED>(PIKA_FORWARD(E, e), PIKA_MOVE(xi));
    }

    template <typename E>
    [[noreturn]] void throw_with_info(E&& e, exception_info const& xi)
    {
        throw_with_info(PIKA_FORWARD(E, e), exception_info(xi));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename E>
    exception_info* get_exception_info(E& e)
    {
        return dynamic_cast<exception_info*>(std::addressof(e));
    }

    template <typename E>
    exception_info const* get_exception_info(E const& e)
    {
        return dynamic_cast<exception_info const*>(std::addressof(e));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename E, typename F>
    auto invoke_with_exception_info(E const& e, F&& f)
        -> decltype(PIKA_FORWARD(F, f)(std::declval<exception_info const*>()))
    {
        return PIKA_FORWARD(F, f)(dynamic_cast<exception_info const*>(std::addressof(e)));
    }

    template <typename F>
    auto invoke_with_exception_info(std::exception_ptr const& p, F&& f)
        -> decltype(PIKA_FORWARD(F, f)(std::declval<exception_info const*>()))
    {
        try
        {
            if (p)
                std::rethrow_exception(p);
        }
        catch (exception_info const& xi)
        {
            return PIKA_FORWARD(F, f)(&xi);
        }
        catch (...)
        {
        }
        return PIKA_FORWARD(F, f)(nullptr);
    }

    template <typename F>
    auto invoke_with_exception_info(pika::error_code const& ec, F&& f)
        -> decltype(PIKA_FORWARD(F, f)(std::declval<exception_info const*>()))
    {
        return invoke_with_exception_info(detail::access_exception(ec), PIKA_FORWARD(F, f));
    }
}    // namespace pika
