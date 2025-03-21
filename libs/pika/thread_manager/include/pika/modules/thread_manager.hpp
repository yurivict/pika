//  Copyright (c) 2007-2020 Hartmut Kaiser
//  Copyright (c) 2007-2009 Chirag Dekate, Anshul Tandon
//  Copyright (c)      2011 Bryce Lelbach, Katelyn Kufahl
//  Copyright (c)      2017 Shoshana Jakobovits
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/concurrency/barrier.hpp>
#include <pika/concurrency/spinlock.hpp>
#include <pika/modules/errors.hpp>
#include <pika/resource_partitioner/detail/partitioner.hpp>
#include <pika/runtime_configuration/runtime_configuration.hpp>
#include <pika/thread_manager/thread_manager_fwd.hpp>
#include <pika/thread_pools/scheduled_thread_pool.hpp>
#include <pika/threading_base/scheduler_mode.hpp>
#include <pika/threading_base/scheduler_state.hpp>
#include <pika/threading_base/thread_init_data.hpp>
#include <pika/threading_base/thread_num_tss.hpp>
#include <pika/threading_base/thread_pool_base.hpp>
#include <pika/topology/cpu_mask.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iosfwd>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <pika/config/warnings_prefix.hpp>

namespace pika::threads::detail {
    ///////////////////////////////////////////////////////////////////////////
    /// The \a thread-manager class is the central instance of management for
    /// all (non-depleted) threads
    class thread_manager
    {
    private:
        // we use a simple mutex to protect the data members of the
        // thread manager for now
        using mutex_type = std::mutex;

    public:
        using notification_policy_type = threads::callback_notifier;
        using pool_type = std::unique_ptr<thread_pool_base>;
        using scheduler_type = threads::detail::scheduler_base;
        using pool_vector = std::vector<pool_type>;

        thread_manager(pika::util::runtime_configuration& rtcfg_,
            notification_policy_type& notifier,
            network_background_callback_type network_background_callback =
                network_background_callback_type());
        ~thread_manager();

        void init();
        void create_pools();

        //! FIXME move to private and add --pika:printpools cmd line option
        void print_pools(std::ostream&);

        // Get functions
        thread_pool_base& default_pool() const;

        scheduler_type& default_scheduler() const;

        thread_pool_base& get_pool(std::string const& pool_name) const;
        thread_pool_base& get_pool(pool_id_type const& pool_id) const;
        thread_pool_base& get_pool(std::size_t thread_index) const;

        bool pool_exists(std::string const& pool_name) const;
        bool pool_exists(std::size_t pool_index) const;

        /// The function \a register_work adds a new work item to the thread
        /// manager. It doesn't immediately create a new \a thread, it just adds
        /// the task parameters (function, initial state and description) to
        /// the internal management data structures. The thread itself will be
        /// created when the number of existing threads drops below the number
        /// of threads specified by the constructors max_count parameter.
        ///
        /// \param func   [in] The function or function object to execute as
        ///               the thread's function. This must have a signature as
        ///               defined by \a thread_function_type.
        /// \param description [in] The value of this parameter allows to
        ///               specify a description of the thread to create. This
        ///               information is used for logging purposes mainly, but
        ///               might be useful for debugging as well. This parameter
        ///               is optional and defaults to an empty string.
        thread_id_ref_type register_work(thread_init_data& data, error_code& ec = throws);

        /// The function \a register_thread adds a new work item to the thread
        /// manager. It creates a new \a thread, adds it to the internal
        /// management data structures, and schedules the new thread, if
        /// appropriate.
        ///
        /// \param func   [in] The function or function object to execute as
        ///               the thread's function. This must have a signature as
        ///               defined by \a thread_function_type.
        /// \param id     [out] This parameter will hold the id of the created
        ///               thread. This id is guaranteed to be validly
        ///               initialized before the thread function is executed.
        /// \param description [in] The value of this parameter allows to
        ///               specify a description of the thread to create. This
        ///               information is used for logging purposes mainly, but
        ///               might be useful for debugging as well. This parameter
        ///               is optional and defaults to an empty string.
        void register_thread(
            thread_init_data& data, thread_id_ref_type& id, error_code& ec = throws);

        /// \brief  Run the thread manager's work queue. This function
        ///         instantiates the specified number of OS threads in each
        ///         pool. All OS threads are started to execute the function
        ///         \a tfunc.
        ///
        /// \returns      The function returns \a true if the thread manager
        ///               has been started successfully, otherwise it returns
        ///               \a false.
        bool run();

        /// \brief Forcefully stop the thread-manager
        ///
        /// \param blocking
        ///
        void stop(bool blocking = true);

        bool is_busy();
        bool is_idle();
        void wait();

        // \brief Suspend all thread pools.
        void suspend();

        // \brief Resume all thread pools.
        void resume();

        /// \brief Return whether the thread manager is still running
        //! This returns the "minimal state", i.e. the state of the
        //! least advanced thread pool
        runtime_state status() const
        {
            pika::runtime_state result(runtime_state::last_valid_runtime);

            for (auto& pool_iter : pools_)
            {
                pika::runtime_state s = pool_iter->get_state();
                result = (std::min)(result, s);
            }

            return result;
        }

        /// \brief return the number of pika-threads with the given state
        ///
        /// \note This function lock the internal OS lock in the thread manager
        std::int64_t get_thread_count(thread_schedule_state state = thread_schedule_state::unknown,
            execution::thread_priority priority = execution::thread_priority::default_,
            std::size_t num_thread = std::size_t(-1), bool reset = false);

        std::int64_t get_idle_core_count();

        mask_type get_idle_core_mask();

        std::int64_t get_background_thread_count();

        // Enumerate all matching threads
        bool enumerate_threads(util::detail::function<bool(thread_id_type)> const& f,
            thread_schedule_state state = thread_schedule_state::unknown) const;

        // \brief Abort all threads which are in suspended state. This will set
        //        the state of all suspended threads to \a pending while
        //        supplying the wait_abort extended state flag
        void abort_all_suspended_threads();

        // \brief Clean up terminated threads. This deletes all threads which
        //        have been terminated but which are still held in the queue
        //        of terminated threads. Some schedulers might not do anything
        //        here.
        bool cleanup_terminated(bool delete_all);

        /// \brief Return the number of OS threads running in this thread-manager
        ///
        /// This function will return correct results only if the thread-manager
        /// is running.
        std::size_t get_os_thread_count() const
        {
            std::lock_guard<mutex_type> lk(mtx_);
            std::size_t total = 0;
            for (auto& pool_iter : pools_)
            {
                total += pool_iter->get_os_thread_count();
            }
            return total;
        }

        std::thread& get_os_thread_handle(std::size_t num_thread) const
        {
            std::lock_guard<mutex_type> lk(mtx_);
            pool_id_type id = threads_lookup_[num_thread];
            thread_pool_base& pool = get_pool(id);
            return pool.get_os_thread_handle(num_thread);
        }

    public:
        /// API functions forwarding to notification policy

        /// This notifies the thread manager that the passed exception has been
        /// raised. The exception will be routed through the notifier and the
        /// scheduler (which will result in it being passed to the runtime
        /// object, which in turn will report it to the console, etc.).
        void report_error(std::size_t num_thread, std::exception_ptr const& e)
        {
            // propagate the error reporting to all pools, which in turn
            // will propagate to schedulers
            for (auto& pool_iter : pools_)
            {
                pool_iter->report_error(num_thread, e);
            }
        }

    public:
        /// Returns the mask identifying all processing units used by this
        /// thread manager.
        mask_type get_used_processing_units() const
        {
            mask_type total_used_processing_punits = mask_type();
            resize(total_used_processing_punits, hardware_concurrency());

            for (auto& pool_iter : pools_)
            {
                total_used_processing_punits |= pool_iter->get_used_processing_units();
            }

            return total_used_processing_punits;
        }

        hwloc_bitmap_ptr get_pool_numa_bitmap(const std::string& pool_name) const
        {
            return get_pool(pool_name).get_numa_domain_bitmap();
        }

        void set_scheduler_mode(threads::scheduler_mode mode)
        {
            for (auto& pool_iter : pools_)
            {
                pool_iter->get_scheduler()->set_scheduler_mode(mode);
            }
        }

        void add_scheduler_mode(threads::scheduler_mode mode)
        {
            for (auto& pool_iter : pools_)
            {
                pool_iter->get_scheduler()->add_scheduler_mode(mode);
            }
        }

        void remove_scheduler_mode(threads::scheduler_mode mode)
        {
            for (auto& pool_iter : pools_)
            {
                pool_iter->get_scheduler()->remove_scheduler_mode(mode);
            }
        }

        void reset_thread_distribution()
        {
            for (auto& pool_iter : pools_)
            {
                pool_iter->reset_thread_distribution();
            }
        }

        void init_tss(std::size_t global_thread_num)
        {
            set_global_thread_num_tss(global_thread_num);
        }

        void deinit_tss()
        {
            set_global_thread_num_tss(std::size_t(-1));
        }

    public:
        // performance counters
        std::int64_t get_queue_length(bool reset);
#ifdef PIKA_HAVE_THREAD_QUEUE_WAITTIME
        std::int64_t get_average_thread_wait_time(bool reset);
        std::int64_t get_average_task_wait_time(bool reset);
#endif

        std::int64_t get_cumulative_duration(bool reset);

        std::int64_t get_thread_count_unknown(bool reset)
        {
            return get_thread_count(thread_schedule_state::unknown,
                execution::thread_priority::default_, std::size_t(-1), reset);
        }
        std::int64_t get_thread_count_active(bool reset)
        {
            return get_thread_count(thread_schedule_state::active,
                execution::thread_priority::default_, std::size_t(-1), reset);
        }
        std::int64_t get_thread_count_pending(bool reset)
        {
            return get_thread_count(thread_schedule_state::pending,
                execution::thread_priority::default_, std::size_t(-1), reset);
        }
        std::int64_t get_thread_count_suspended(bool reset)
        {
            return get_thread_count(thread_schedule_state::suspended,
                execution::thread_priority::default_, std::size_t(-1), reset);
        }
        std::int64_t get_thread_count_terminated(bool reset)
        {
            return get_thread_count(thread_schedule_state::terminated,
                execution::thread_priority::default_, std::size_t(-1), reset);
        }
        std::int64_t get_thread_count_staged(bool reset)
        {
            return get_thread_count(thread_schedule_state::staged,
                execution::thread_priority::default_, std::size_t(-1), reset);
        }

#ifdef PIKA_HAVE_THREAD_IDLE_RATES
        std::int64_t avg_idle_rate(bool reset);
# ifdef PIKA_HAVE_THREAD_CREATION_AND_CLEANUP_RATES
        std::int64_t avg_creation_idle_rate(bool reset);
        std::int64_t avg_cleanup_idle_rate(bool reset);
# endif
#endif

#ifdef PIKA_HAVE_THREAD_CUMULATIVE_COUNTS
        std::int64_t get_executed_threads(bool reset);
        std::int64_t get_executed_thread_phases(bool reset);
# ifdef PIKA_HAVE_THREAD_IDLE_RATES
        std::int64_t get_thread_duration(bool reset);
        std::int64_t get_thread_phase_duration(bool reset);
        std::int64_t get_thread_overhead(bool reset);
        std::int64_t get_thread_phase_overhead(bool reset);
        std::int64_t get_cumulative_thread_duration(bool reset);
        std::int64_t get_cumulative_thread_overhead(bool reset);
# endif
#endif

#ifdef PIKA_HAVE_THREAD_STEALING_COUNTS
        std::int64_t get_num_pending_misses(bool reset);
        std::int64_t get_num_pending_accesses(bool reset);
        std::int64_t get_num_stolen_from_pending(bool reset);
        std::int64_t get_num_stolen_from_staged(bool reset);
        std::int64_t get_num_stolen_to_pending(bool reset);
        std::int64_t get_num_stolen_to_staged(bool reset);
#endif

    private:
        mutable mutex_type mtx_;    // mutex protecting the members

        pika::util::runtime_configuration& rtcfg_;
        std::vector<pool_id_type> threads_lookup_;

        pool_vector pools_;

        notification_policy_type& notifier_;
        network_background_callback_type network_background_callback_;
    };
}    // namespace pika::threads::detail

#include <pika/config/warnings_suffix.hpp>
