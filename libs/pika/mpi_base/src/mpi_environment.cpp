//  Copyright (c) 2013-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <pika/config.hpp>

#include <pika/modules/logging.hpp>
#include <pika/modules/mpi_base.hpp>
#include <pika/modules/runtime_configuration.hpp>
#include <pika/modules/util.hpp>

#include <boost/tokenizer.hpp>

#include <cstddef>
#include <cstdlib>
#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace pika { namespace util {

    namespace detail {

        bool detect_mpi_environment(util::runtime_configuration const& cfg, char const* default_env)
        {
#if defined(__bgq__)
            // If running on BG/Q, we can safely assume to always run in an
            // MPI environment
            return true;
#else
            std::string mpi_environment_strings = cfg.get_entry("pika.parcel.mpi.env", default_env);

            boost::char_separator<char> sep(";,: ");
            boost::tokenizer<boost::char_separator<char>> tokens(mpi_environment_strings, sep);
            for (auto const& tok : tokens)
            {
                char* env = std::getenv(tok.c_str());
                if (env)
                {
                    LBT_(debug) << "Found MPI environment variable: " << tok << "="
                                << std::string(env) << ", enabling MPI support\n";
                    return true;
                }
            }

            LBT_(info) << "No known MPI environment variable found, disabling MPI support\n";
            return false;
#endif
        }
    }    // namespace detail

    bool mpi_environment::check_mpi_environment(util::runtime_configuration const& cfg)
    {
        // log message was already generated
        return detail::detect_mpi_environment(cfg, PIKA_HAVE_MPI_ENV);
    }
}}    // namespace pika::util

#if defined(PIKA_HAVE_MODULE_MPI_BASE)

namespace pika { namespace util {

    mpi_environment::mutex_type mpi_environment::mtx_;
    bool mpi_environment::enabled_ = false;
    bool mpi_environment::has_called_init_ = false;
    int mpi_environment::provided_threading_flag_ = MPI_THREAD_SINGLE;
    MPI_Comm mpi_environment::communicator_ = MPI_COMM_NULL;

    int mpi_environment::is_initialized_ = -1;

    ///////////////////////////////////////////////////////////////////////////
    int mpi_environment::init(int*, char***, const int required, const int minimal, int& provided)
    {
        has_called_init_ = false;

        // Check if MPI_Init has been called previously
        int is_initialized = 0;
        int retval = MPI_Initialized(&is_initialized);
        if (MPI_SUCCESS != retval)
        {
            return retval;
        }
        if (!is_initialized)
        {
            retval = MPI_Init_thread(nullptr, nullptr, required, &provided);
            if (MPI_SUCCESS != retval)
            {
                return retval;
            }

            if (provided < minimal)
            {
                PIKA_THROW_EXCEPTION(pika::error::invalid_status,
                    "pika::util::mpi_environment::init",
                    "MPI doesn't provide minimal requested thread level");
            }
            has_called_init_ = true;
        }
        return retval;
    }

    ///////////////////////////////////////////////////////////////////////////
    void mpi_environment::init(int* argc, char*** argv, util::runtime_configuration& rtcfg)
    {
        if (enabled_)
            return;    // don't call twice

        int this_rank = -1;
        has_called_init_ = false;

        enabled_ = check_mpi_environment(rtcfg);
        if (!enabled_)
        {
            return;
        }

        int required = MPI_THREAD_SINGLE;
        int minimal = MPI_THREAD_SINGLE;
# if defined(PIKA_HAVE_MPI_MULTITHREADED)
        required = (pika::detail::get_entry_as(rtcfg, "pika.parcel.mpi.multithreaded", 1) != 0) ?
            MPI_THREAD_MULTIPLE :
            MPI_THREAD_SINGLE;

#  if defined(MVAPICH2_VERSION) && defined(_POSIX_SOURCE)
        // This enables multi threading support in MVAPICH2 if requested.
        if (required == MPI_THREAD_MULTIPLE)
            setenv("MV2_ENABLE_AFFINITY", "0", 1);
#  endif
# endif

        int retval = init(argc, argv, required, minimal, provided_threading_flag_);
        if (MPI_SUCCESS != retval && MPI_ERR_OTHER != retval)
        {
            enabled_ = false;

            int msglen = 0;
            char message[MPI_MAX_ERROR_STRING + 1];
            MPI_Error_string(retval, message, &msglen);
            message[msglen] = '\0';

            std::string msg("mpi_environment::init: MPI_Init_thread failed: ");
            msg = msg + message + ".";
            throw std::runtime_error(msg.c_str());
        }

        MPI_Comm_dup(MPI_COMM_WORLD, &communicator_);

        if (provided_threading_flag_ < MPI_THREAD_SERIALIZED)
        {
            // explicitly disable mpi if not run by mpirun
            rtcfg.add_entry("pika.parcel.mpi.multithreaded", "0");
        }

        if (provided_threading_flag_ == MPI_THREAD_FUNNELED)
        {
            enabled_ = false;
            has_called_init_ = false;
            throw std::runtime_error(
                "mpi_environment::init: MPI_Init_thread: The underlying MPI implementation only "
                "supports MPI_THREAD_FUNNELED. This mode is not supported by pika. Please pass "
                "-Ipika.parcel.mpi.multithreaded=0 to explicitly disable MPI multi-threading.");
        }

        this_rank = rank();

        if (this_rank == 0)
        {
            rtcfg.mode_ = pika::runtime_mode::console;
        }
        else
        {
            rtcfg.mode_ = pika::runtime_mode::worker;
        }

        rtcfg.add_entry("pika.parcel.mpi.rank", std::to_string(this_rank));
        rtcfg.add_entry("pika.parcel.mpi.processorname", get_processor_name());
    }

    std::string mpi_environment::get_processor_name()
    {
        char name[MPI_MAX_PROCESSOR_NAME + 1] = {'\0'};
        int len = 0;
        MPI_Get_processor_name(name, &len);

        return name;
    }

    void mpi_environment::finalize()
    {
        if (enabled() && has_called_init())
        {
            int is_finalized = 0;
            MPI_Finalized(&is_finalized);
            if (!is_finalized)
            {
                MPI_Finalize();
            }
        }
    }

    bool mpi_environment::enabled()
    {
        return enabled_;
    }

    bool mpi_environment::multi_threaded()
    {
        return provided_threading_flag_ >= MPI_THREAD_SERIALIZED;
    }

    bool mpi_environment::has_called_init()
    {
        return has_called_init_;
    }

    int mpi_environment::size()
    {
        int res(-1);
        if (enabled())
            MPI_Comm_size(communicator(), &res);
        return res;
    }

    int mpi_environment::rank()
    {
        int res(-1);
        if (enabled())
            MPI_Comm_rank(communicator(), &res);
        return res;
    }

    MPI_Comm& mpi_environment::communicator()
    {
        return communicator_;
    }

    mpi_environment::scoped_lock::scoped_lock()
    {
        if (!multi_threaded())
            mtx_.lock();
    }

    mpi_environment::scoped_lock::~scoped_lock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    void mpi_environment::scoped_lock::unlock()
    {
        if (!multi_threaded())
            mtx_.unlock();
    }

    mpi_environment::scoped_try_lock::scoped_try_lock()
      : locked(true)
    {
        if (!multi_threaded())
        {
            locked = mtx_.try_lock();
        }
    }

    mpi_environment::scoped_try_lock::~scoped_try_lock()
    {
        if (!multi_threaded() && locked)
            mtx_.unlock();
    }

    void mpi_environment::scoped_try_lock::unlock()
    {
        if (!multi_threaded() && locked)
        {
            locked = false;
            mtx_.unlock();
        }
    }
}}    // namespace pika::util

#endif
