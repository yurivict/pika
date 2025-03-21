//  Copyright (c) 2014 Hartmut Kaiser
//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/assert.hpp>
#include <pika/modules/errors.hpp>
#include <pika/modules/futures.hpp>
#include <pika/synchronization/no_mutex.hpp>
#include <pika/synchronization/spinlock.hpp>

#include <cstddef>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace pika::lcos::local {
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Mutex = pika::spinlock>
    struct receive_buffer
    {
    protected:
        using mutex_type = Mutex;
        using buffer_promise_type = pika::lcos::local::promise<T>;

        struct entry_data
        {
        public:
            PIKA_NON_COPYABLE(entry_data);

        public:
            entry_data()
              : can_be_deleted_(false)
              , value_set_(false)
            {
            }

            pika::future<T> get_future()
            {
                return promise_.get_future();
            }

            template <typename Val>
            void set_value(Val&& val)
            {
                value_set_ = true;
                promise_.set_value(PIKA_FORWARD(Val, val));
            }

            bool cancel(std::exception_ptr const& e)
            {
                PIKA_ASSERT(can_be_deleted_);
                if (!value_set_)
                {
                    promise_.set_exception(e);
                    return true;
                }
                return false;
            }

            buffer_promise_type promise_;
            bool can_be_deleted_;
            bool value_set_;
        };

        using buffer_map_type = std::map<std::size_t, std::shared_ptr<entry_data>>;
        using iterator = typename buffer_map_type::iterator;

        struct erase_on_exit
        {
            erase_on_exit(buffer_map_type& buffer_map, iterator it)
              : buffer_map_(buffer_map)
              , it_(it)
            {
            }

            ~erase_on_exit()
            {
                buffer_map_.erase(it_);
            }

            buffer_map_type& buffer_map_;
            iterator it_;
        };

    public:
        receive_buffer() = default;

        receive_buffer(receive_buffer&& other) noexcept
          : mtx_()
          , buffer_map_(PIKA_MOVE(other.buffer_map_))
        {
        }

        ~receive_buffer()
        {
            PIKA_ASSERT(buffer_map_.empty());
        }

        receive_buffer& operator=(receive_buffer&& other) noexcept
        {
            if (this != &other)
            {
                mtx_ = mutex_type();
                buffer_map_ = PIKA_MOVE(other.buffer_map_);
            }
            return *this;
        }

        pika::future<T> receive(std::size_t step)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = get_buffer_entry(step);
            PIKA_ASSERT(it != buffer_map_.end());

            // if the value was already set we delete the entry after
            // retrieving the future
            if (it->second->can_be_deleted_)
            {
                erase_on_exit t(buffer_map_, it);
                return it->second->get_future();
            }

            // otherwise mark the entry as to be deleted once the value was set
            it->second->can_be_deleted_ = true;
            return it->second->get_future();
        }

        bool try_receive(std::size_t step, pika::future<T>* f = nullptr)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
                return false;

            // if the value was already set we delete the entry after
            // retrieving the future
            if (it->second->can_be_deleted_)
            {
                if (f != nullptr)
                {
                    erase_on_exit t(buffer_map_, it);
                    *f = it->second->get_future();
                }
                return true;
            }

            // otherwise mark the entry as to be deleted once the value was set
            if (f != nullptr)
            {
                it->second->can_be_deleted_ = true;
                *f = it->second->get_future();
            }
            return true;
        }

        template <typename Lock = pika::no_mutex>
        void store_received(std::size_t step, T&& val, Lock* lock = nullptr)
        {
            std::shared_ptr<entry_data> entry;

            {
                std::lock_guard<mutex_type> l(mtx_);

                iterator it = get_buffer_entry(step);
                PIKA_ASSERT(it != buffer_map_.end());

                entry = it->second;

                if (!entry->can_be_deleted_)
                {
                    // if the future was not retrieved yet mark the entry as
                    // to be deleted after it was be retrieved
                    entry->can_be_deleted_ = true;
                }
                else
                {
                    // if the future was already retrieved we can delete the
                    // entry now
                    buffer_map_.erase(it);
                }
            }

            if (lock)
                lock->unlock();

            // set value in promise, but only after the lock went out of scope
            entry->set_value(PIKA_MOVE(val));
        }

        bool empty() const
        {
            return buffer_map_.empty();
        }

        // return the number of deleted buffer entries
        std::size_t cancel_waiting(std::exception_ptr const& e, bool force_delete_entries = false)
        {
            std::lock_guard<mutex_type> l(mtx_);

            std::size_t count = 0;
            iterator end = buffer_map_.end();
            for (iterator it = buffer_map_.begin(); it != end; /**/)
            {
                iterator to_delete = it++;
                if (to_delete->second->cancel(e) || force_delete_entries)
                {
                    buffer_map_.erase(to_delete);
                    ++count;
                }
            }
            return count;
        }

    protected:
        iterator get_buffer_entry(std::size_t step)
        {
            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
            {
                std::pair<iterator, bool> res =
                    buffer_map_.insert(std::make_pair(step, std::make_shared<entry_data>()));
                if (!res.second)
                {
                    PIKA_THROW_EXCEPTION(pika::error::invalid_status,
                        "base_receive_buffer::get_buffer_entry",
                        "couldn't insert a new entry into the receive buffer");
                }
                return res.first;
            }
            return it;
        }

    private:
        mutable mutex_type mtx_;
        buffer_map_type buffer_map_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Mutex>
    struct receive_buffer<void, Mutex>
    {
    protected:
        using mutex_type = Mutex;
        using buffer_promise_type = pika::lcos::local::promise<void>;

        struct entry_data
        {
        public:
            PIKA_NON_COPYABLE(entry_data);

        public:
            entry_data()
              : can_be_deleted_(false)
              , value_set_(false)
            {
            }

            pika::future<void> get_future()
            {
                return promise_.get_future();
            }

            void set_value()
            {
                value_set_ = true;
                promise_.set_value();
            }

            bool cancel(std::exception_ptr const& e)
            {
                PIKA_ASSERT(can_be_deleted_);
                if (!value_set_)
                {
                    promise_.set_exception(e);
                    return true;
                }
                return false;
            }

            buffer_promise_type promise_;
            bool can_be_deleted_;
            bool value_set_;
        };

        using buffer_map_type = std::map<std::size_t, std::shared_ptr<entry_data>>;
        using iterator = typename buffer_map_type::iterator;

        struct erase_on_exit
        {
            erase_on_exit(buffer_map_type& buffer_map, iterator it)
              : buffer_map_(buffer_map)
              , it_(it)
            {
            }

            ~erase_on_exit()
            {
                buffer_map_.erase(it_);
            }

            buffer_map_type& buffer_map_;
            iterator it_;
        };

    public:
        receive_buffer() {}

        receive_buffer(receive_buffer&& other)
          : buffer_map_(PIKA_MOVE(other.buffer_map_))
        {
        }

        ~receive_buffer()
        {
            PIKA_ASSERT(buffer_map_.empty());
        }

        receive_buffer& operator=(receive_buffer&& other)
        {
            if (this != &other)
            {
                buffer_map_ = PIKA_MOVE(other.buffer_map_);
            }
            return *this;
        }

        pika::future<void> receive(std::size_t step)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = get_buffer_entry(step);
            PIKA_ASSERT(it != buffer_map_.end());

            // if the value was already set we delete the entry after
            // retrieving the future
            if (it->second->can_be_deleted_)
            {
                erase_on_exit t(buffer_map_, it);
                return it->second->get_future();
            }

            // otherwise mark the entry as to be deleted once the value was set
            it->second->can_be_deleted_ = true;
            return it->second->get_future();
        }

        bool try_receive(std::size_t step, pika::future<void>* f = nullptr)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
                return false;

            // if the value was already set we delete the entry after
            // retrieving the future
            if (it->second->can_be_deleted_)
            {
                if (f != nullptr)
                {
                    erase_on_exit t(buffer_map_, it);
                    *f = it->second->get_future();
                }
                return true;
            }

            // otherwise mark the entry as to be deleted once the value was set
            if (f != nullptr)
            {
                it->second->can_be_deleted_ = true;
                *f = it->second->get_future();
            }
            return true;
        }

        template <typename Lock = pika::no_mutex>
        void store_received(std::size_t step, Lock* lock = nullptr)
        {
            std::shared_ptr<entry_data> entry;

            {
                std::lock_guard<mutex_type> l(mtx_);

                iterator it = get_buffer_entry(step);
                PIKA_ASSERT(it != buffer_map_.end());

                entry = it->second;

                if (!entry->can_be_deleted_)
                {
                    // if the future was not retrieved yet mark the entry as
                    // to be deleted after it was be retrieved
                    entry->can_be_deleted_ = true;
                }
                else
                {
                    // if the future was already retrieved we can delete the
                    // entry now
                    buffer_map_.erase(it);
                }
            }

            if (lock)
                lock->unlock();

            // set value in promise, but only after the lock went out of scope
            entry->set_value();
        }

        bool empty() const
        {
            return buffer_map_.empty();
        }

        // return the number of deleted buffer entries
        std::size_t cancel_waiting(std::exception_ptr const& e, bool force_delete_entries = false)
        {
            std::lock_guard<mutex_type> l(mtx_);

            std::size_t count = 0;
            iterator end = buffer_map_.end();
            for (iterator it = buffer_map_.begin(); it != end; /**/)
            {
                iterator to_delete = it++;
                if (to_delete->second->cancel(e) || force_delete_entries)
                {
                    buffer_map_.erase(to_delete);
                    ++count;
                }
            }
            return count;
        }

    protected:
        iterator get_buffer_entry(std::size_t step)
        {
            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
            {
                std::pair<iterator, bool> res =
                    buffer_map_.insert(std::make_pair(step, std::make_shared<entry_data>()));
                if (!res.second)
                {
                    PIKA_THROW_EXCEPTION(pika::error::invalid_status,
                        "base_receive_buffer::get_buffer_entry",
                        "couldn't insert a new entry into the receive buffer");
                }
                return res.first;
            }
            return it;
        }

    private:
        mutable mutex_type mtx_;
        buffer_map_type buffer_map_;
    };
}    // namespace pika::lcos::local
