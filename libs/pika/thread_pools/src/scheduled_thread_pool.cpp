//  Copyright (c) 2017 Shoshana Jakobovits
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>
#include <pika/schedulers/local_priority_queue_scheduler.hpp>
#include <pika/schedulers/local_queue_scheduler.hpp>
#include <pika/schedulers/shared_priority_queue_scheduler.hpp>
#include <pika/schedulers/static_priority_queue_scheduler.hpp>
#include <pika/schedulers/static_queue_scheduler.hpp>
#include <pika/thread_pools/scheduled_thread_pool.hpp>
#include <pika/thread_pools/scheduled_thread_pool_impl.hpp>

///////////////////////////////////////////////////////////////////////////////
/// explicit template instantiation for the thread pools of our choice
template class PIKA_EXPORT pika::threads::local_queue_scheduler<>;
template class PIKA_EXPORT
    pika::threads::detail::scheduled_thread_pool<pika::threads::local_queue_scheduler<>>;

template class PIKA_EXPORT pika::threads::static_queue_scheduler<>;
template class PIKA_EXPORT
    pika::threads::detail::scheduled_thread_pool<pika::threads::static_queue_scheduler<>>;

template class PIKA_EXPORT pika::threads::local_priority_queue_scheduler<>;
template class PIKA_EXPORT pika::threads::detail::scheduled_thread_pool<
    pika::threads::local_priority_queue_scheduler<std::mutex, pika::threads::lockfree_fifo>>;

template class PIKA_EXPORT pika::threads::static_priority_queue_scheduler<>;
template class PIKA_EXPORT
    pika::threads::detail::scheduled_thread_pool<pika::threads::static_priority_queue_scheduler<>>;
#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
template class PIKA_EXPORT
    pika::threads::local_priority_queue_scheduler<std::mutex, pika::threads::lockfree_lifo>;
template class PIKA_EXPORT pika::threads::detail::scheduled_thread_pool<
    pika::threads::local_priority_queue_scheduler<std::mutex, pika::threads::lockfree_lifo>>;
#endif

#if defined(PIKA_HAVE_CXX11_STD_ATOMIC_128BIT)
template class PIKA_EXPORT
    pika::threads::local_priority_queue_scheduler<std::mutex, pika::threads::lockfree_abp_fifo>;
template class PIKA_EXPORT pika::threads::detail::scheduled_thread_pool<
    pika::threads::local_priority_queue_scheduler<std::mutex, pika::threads::lockfree_abp_fifo>>;
template class PIKA_EXPORT
    pika::threads::local_priority_queue_scheduler<std::mutex, pika::threads::lockfree_abp_lifo>;
template class PIKA_EXPORT pika::threads::detail::scheduled_thread_pool<
    pika::threads::local_priority_queue_scheduler<std::mutex, pika::threads::lockfree_abp_lifo>>;
#endif

template class PIKA_EXPORT pika::threads::shared_priority_queue_scheduler<>;
template class PIKA_EXPORT
    pika::threads::detail::scheduled_thread_pool<pika::threads::shared_priority_queue_scheduler<>>;
