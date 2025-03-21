//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <pika/config.hpp>
#include <pika/async_base/traits/is_launch_policy.hpp>
#include <pika/execution/executors/execution_parameters.hpp>
#include <pika/execution/executors/rebind_executor.hpp>
#include <pika/execution/traits/executor_traits.hpp>
#include <pika/execution/traits/is_execution_policy.hpp>
#include <pika/execution_base/traits/is_executor.hpp>
#include <pika/executors/datapar/execution_policy_fwd.hpp>
#include <pika/executors/execution_policy.hpp>
#include <pika/executors/parallel_executor.hpp>
#include <pika/executors/sequenced_executor.hpp>

#include <type_traits>
#include <utility>

namespace pika::execution {
    ///////////////////////////////////////////////////////////////////////////
    /// Extension: The class simd_task_policy is an execution
    /// policy type used as a unique type to disambiguate parallel algorithm
    /// overloading and indicate that a parallel algorithm's execution may not
    /// be parallelized (has to run sequentially).
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the
    /// sequenced_policy.
    struct simd_task_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = sequenced_executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        typedef parallel::execution::extract_executor_parameters<executor_type>::type
            executor_parameters_type;

        /// The category of the execution agents created by this execution
        /// policy.
        using execution_category = unsequenced_execution_tag;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = simd_task_policy_shim<Executor_, Parameters_>;
        };

        /// \cond NOINTERNAL
        constexpr simd_task_policy() = default;
        /// \endcond

        /// Create a new simd_task_policy from itself
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new sequenced_task_policy
        ///
        constexpr simd_task_policy operator()(task_policy_tag) const
        {
            return *this;
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task simd policy
        ///
        inline decltype(auto) operator()(non_task_policy_tag /*tag*/) const;

        /// Create a new simd_task_policy from the given
        /// executor
        ///
        /// \tparam Executor    The type of the executor to associate with this
        ///                     execution policy.
        ///
        /// \param exec         [in] The executor to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor<Executor>::value is true
        ///
        /// \returns The new simd_task_policy
        ///
        template <typename Executor>
        typename parallel::execution::rebind_executor<simd_task_policy, Executor,
            executor_parameters_type>::type
        on(Executor&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor>::value,
                "pika::traits::is_executor_any<Executor>::value");

            typedef typename parallel::execution::rebind_executor<simd_task_policy, Executor,
                executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor, exec), parameters());
        }

        /// Create a new simd_task_policy from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: all parameters are executor_parameters,
        ///                 different parameter types can't be duplicated
        ///
        /// \returns The new simd_task_policy
        ///
        template <typename... Parameters,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters...>::type>
        typename parallel::execution::rebind_executor<simd_task_policy, executor_type,
            ParametersType>::type
        with(Parameters&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<simd_task_policy, executor_type,
                ParametersType>::type rebound_type;
            return rebound_type(executor(),
                parallel::execution::join_executor_parameters(PIKA_FORWARD(Parameters, params)...));
        }

        using base_policy_type = sequenced_task_policy;
        base_policy_type base_policy()
        {
            return sequenced_task_policy{};
        }

    public:
        /// Return the associated executor object.
        executor_type& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr executor_type const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        executor_parameters_type& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr executor_parameters_type const& parameters() const
        {
            return params_;
        }

    private:
        executor_type exec_;
        executor_parameters_type params_;
    };

    /// Extension: The class simd_task_policy_shim is an
    /// execution policy type used as a unique type to disambiguate parallel
    /// algorithm overloading based on combining a underlying
    /// \a sequenced_task_policy and an executor and indicate that
    /// a parallel algorithm's execution may not be parallelized  (has to run
    /// sequentially).
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the
    /// sequenced_policy.
    template <typename Executor, typename Parameters>
    struct simd_task_policy_shim : simd_task_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = Executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        using executor_parameters_type = Parameters;

        /// The category of the execution agents created by this execution
        /// policy.
        typedef typename pika::traits::executor_execution_category<executor_type>::type
            execution_category;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = simd_task_policy_shim<Executor_, Parameters_>;
        };

        /// Create a new simd_task_policy_shim from itself
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new sequenced_task_policy
        ///
        constexpr simd_task_policy_shim const& operator()(task_policy_tag) const
        {
            return *this;
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task simd_policy_shim
        ///
        inline decltype(auto) operator()(non_task_policy_tag /*tag*/) const;

        /// Create a new simd_task_policy_shim from the given
        /// executor
        ///
        /// \tparam Executor    The type of the executor to associate with this
        ///                     execution policy.
        ///
        /// \param exec         [in] The executor to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor<Executor>::value is true
        ///
        /// \returns The new simd_task_policy_shim
        ///
        template <typename Executor_>
        typename parallel::execution::rebind_executor<simd_task_policy_shim, Executor_,
            executor_parameters_type>::type
        on(Executor_&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor_>::value,
                "pika::traits::is_executor_any<Executor_>::value");

            typedef typename parallel::execution::rebind_executor<simd_task_policy_shim, Executor_,
                executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor_, exec), params_);
        }

        /// Create a new simd_task_policy_shim from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: all parameters are executor_parameters,
        ///                 different parameter types can't be duplicated
        ///
        /// \returns The new sequenced_task_policy_shim
        ///
        template <typename... Parameters_,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters_...>::type>
        typename parallel::execution::rebind_executor<simd_task_policy_shim, executor_type,
            ParametersType>::type
        with(Parameters_&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<simd_task_policy_shim,
                executor_type, ParametersType>::type rebound_type;
            return rebound_type(exec_,
                parallel::execution::join_executor_parameters(
                    PIKA_FORWARD(Parameters_, params)...));
        }

        using base_policy_type = sequenced_task_policy_shim<Executor, Parameters>;
        base_policy_type base_policy()
        {
            return simd_task_policy_shim<Executor, Parameters>(exec_, params_);
        }

        /// Return the associated executor object.
        Executor& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr Executor const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        Parameters& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr Parameters const& parameters() const
        {
            return params_;
        }

        /// \cond NOINTERNAL
        constexpr simd_task_policy_shim() = default;

        template <typename Executor_, typename Parameters_>
        constexpr simd_task_policy_shim(Executor_&& exec, Parameters_&& params)
          : exec_(PIKA_FORWARD(Executor_, exec))
          , params_(PIKA_FORWARD(Parameters_, params))
        {
        }

    private:
        Executor exec_;
        Parameters params_;
        /// \endcond
    };

    ///////////////////////////////////////////////////////////////////////////
    /// The class simd_policy is an execution policy type used
    /// as a unique type to disambiguate parallel algorithm overloading and
    /// require that a parallel algorithm's execution may not be parallelized.
    struct simd_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = sequenced_executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        typedef parallel::execution::extract_executor_parameters<executor_type>::type
            executor_parameters_type;

        /// The category of the execution agents created by this execution
        /// policy.
        using execution_category = unsequenced_execution_tag;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = simd_policy_shim<Executor_, Parameters_>;
        };

        /// \cond NOINTERNAL
        constexpr simd_policy() = default;
        /// \endcond

        /// Create a new simd_task_policy.
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new simd_task_policy
        ///
        constexpr simd_task_policy operator()(task_policy_tag) const
        {
            return simd_task_policy();
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task simd policy
        ///
        constexpr decltype(auto) operator()(non_task_policy_tag /*tag*/) const
        {
            return *this;
        }

        /// Create a new simd_policy from the given
        /// executor
        ///
        /// \tparam Executor    The type of the executor to associate with this
        ///                     execution policy.
        ///
        /// \param exec         [in] The executor to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor<Executor>::value is true
        ///
        /// \returns The new simd_policy
        ///
        template <typename Executor>
        typename parallel::execution::rebind_executor<simd_policy, Executor,
            executor_parameters_type>::type
        on(Executor&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor>::value,
                "pika::traits::is_executor_any<Executor>::value");

            typedef typename parallel::execution::rebind_executor<simd_policy, Executor,
                executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor, exec), parameters());
        }

        /// Create a new simd_policy from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: all parameters are executor_parameters,
        ///                 different parameter types can't be duplicated
        ///
        /// \returns The new simd_policy
        ///
        template <typename... Parameters,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters...>::type>
        typename parallel::execution::rebind_executor<simd_policy, executor_type,
            ParametersType>::type
        with(Parameters&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<simd_policy, executor_type,
                ParametersType>::type rebound_type;
            return rebound_type(executor(),
                parallel::execution::join_executor_parameters(PIKA_FORWARD(Parameters, params)...));
        }

        using base_policy_type = sequenced_policy;
        base_policy_type base_policy()
        {
            return sequenced_policy{};
        }

    public:
        /// Return the associated executor object.
        /// Return the associated executor object.
        executor_type& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr executor_type const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        executor_parameters_type& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr executor_parameters_type const& parameters() const
        {
            return params_;
        }

    private:
        executor_type exec_;
        executor_parameters_type params_;
    };

    /// Default sequential execution policy object.
    static constexpr simd_policy simd{};

    /// The class simd_policy is an execution policy type used
    /// as a unique type to disambiguate parallel algorithm overloading and
    /// require that a parallel algorithm's execution may not be parallelized.
    template <typename Executor, typename Parameters>
    struct simd_policy_shim : simd_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = Executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        using executor_parameters_type = Parameters;

        /// The category of the execution agents created by this execution
        /// policy.
        typedef typename pika::traits::executor_execution_category<executor_type>::type
            execution_category;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = simd_policy_shim<Executor_, Parameters_>;
        };

        /// Create a new simd_task_policy.
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new simd_task_policy_shim
        ///
        constexpr simd_task_policy_shim<Executor, Parameters> operator()(task_policy_tag) const
        {
            return simd_task_policy_shim<Executor, Parameters>(exec_, params_);
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task simd_policy_shim
        ///
        constexpr decltype(auto) operator()(non_task_policy_tag /*tag*/) const
        {
            return *this;
        }

        /// Create a new simd_policy from the given
        /// executor
        ///
        /// \tparam Executor    The type of the executor to associate with this
        ///                     execution policy.
        ///
        /// \param exec         [in] The executor to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor<Executor>::value is true
        ///
        /// \returns The new simd_policy
        ///
        template <typename Executor_>
        typename parallel::execution::rebind_executor<simd_policy_shim, Executor_,
            executor_parameters_type>::type
        on(Executor_&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor_>::value,
                "pika::traits::is_executor_any<Executor_>::value");

            typedef typename parallel::execution::rebind_executor<simd_policy_shim, Executor_,
                executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor_, exec), params_);
        }

        /// Create a new simd_policy_shim from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: all parameters are executor_parameters,
        ///                 different parameter types can't be duplicated
        ///
        /// \returns The new simd_policy_shim
        ///
        template <typename... Parameters_,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters_...>::type>
        typename parallel::execution::rebind_executor<simd_policy_shim, executor_type,
            ParametersType>::type
        with(Parameters_&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<simd_policy_shim, executor_type,
                ParametersType>::type rebound_type;
            return rebound_type(exec_,
                parallel::execution::join_executor_parameters(
                    PIKA_FORWARD(Parameters_, params)...));
        }

        using base_policy_type = sequenced_policy_shim<Executor, Parameters>;
        base_policy_type base_policy()
        {
            return sequenced_policy_shim<Executor, Parameters>(exec_, params_);
        }

        /// Return the associated executor object.
        Executor& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr Executor const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        Parameters& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr Parameters const& parameters() const
        {
            return params_;
        }

        /// \cond NOINTERNAL
        constexpr simd_policy_shim() = default;

        template <typename Executor_, typename Parameters_>
        constexpr simd_policy_shim(Executor_&& exec, Parameters_&& params)
          : exec_(PIKA_FORWARD(Executor_, exec))
          , params_(PIKA_FORWARD(Parameters_, params))
        {
        }

    private:
        Executor exec_;
        Parameters params_;
        /// \endcond
    };

    ///////////////////////////////////////////////////////////////////////////
    /// Extension: The class par_simd_task_policy is an execution
    /// policy type used as a unique type to disambiguate parallel algorithm
    /// overloading and indicate that a parallel algorithm's execution may be
    /// parallelized.
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the parallel_policy.
    struct par_simd_task_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = parallel_executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        typedef parallel::execution::extract_executor_parameters<executor_type>::type
            executor_parameters_type;

        /// The category of the execution agents created by this execution
        /// policy.
        using execution_category = unsequenced_execution_tag;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = par_simd_task_policy_shim<Executor_, Parameters_>;
        };

        /// \cond NOINTERNAL
        constexpr par_simd_task_policy() = default;
        /// \endcond

        /// Create a new par_simd_task_policy from itself
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new par_simd_task_policy
        ///
        constexpr par_simd_task_policy operator()(task_policy_tag) const
        {
            return *this;
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task par_simd policy
        ///
        inline decltype(auto) operator()(non_task_policy_tag /*tag*/) const;

        /// Create a new par_simd_task_policy from given executor
        ///
        /// \tparam Executor    The type of the executor to associate with this
        ///                     execution policy.
        ///
        /// \param exec         [in] The executor to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor<Executor>::value is true
        ///
        /// \returns The new par_simd_task_policy
        ///
        template <typename Executor>
        typename parallel::execution::rebind_executor<par_simd_task_policy, Executor,
            executor_parameters_type>::type
        on(Executor&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor>::value,
                "pika::traits::is_executor_any<Executor>::value");

            typedef typename parallel::execution::rebind_executor<par_simd_task_policy, Executor,
                executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor, exec), parameters());
        }

        /// Create a new par_simd_task_policy_shim from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: all parameters are executor_parameters,
        ///                 different parameter types can't be duplicated
        ///
        /// \returns The new par_simd_policy_shim
        ///
        template <typename... Parameters,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters...>::type>
        typename parallel::execution::rebind_executor<par_simd_task_policy, executor_type,
            ParametersType>::type
        with(Parameters&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<par_simd_task_policy,
                executor_type, ParametersType>::type rebound_type;
            return rebound_type(executor(),
                parallel::execution::join_executor_parameters(PIKA_FORWARD(Parameters, params)...));
        }

        using base_policy_type = parallel_task_policy;
        base_policy_type base_policy()
        {
            return parallel_task_policy{};
        }

    public:
        /// Return the associated executor object.
        executor_type& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr executor_type const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        executor_parameters_type& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr executor_parameters_type const& parameters() const
        {
            return params_;
        }

    private:
        executor_type exec_;
        executor_parameters_type params_;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// The class par_simd_policy is an execution policy type used
    /// as a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may be parallelized.
    struct par_simd_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = parallel_executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        typedef parallel::execution::extract_executor_parameters<executor_type>::type
            executor_parameters_type;

        /// The category of the execution agents created by this execution
        /// policy.
        using execution_category = unsequenced_execution_tag;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = par_simd_policy_shim<Executor_, Parameters_>;
        };

        /// \cond NOINTERNAL
        constexpr par_simd_policy() = default;
        /// \endcond

        /// Create a new par_simd_policy referencing a chunk size.
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new par_simd_task_policy
        ///
        constexpr par_simd_task_policy operator()(task_policy_tag) const
        {
            return par_simd_task_policy();
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task par_simd policy
        ///
        constexpr decltype(auto) operator()(non_task_policy_tag /*tag*/) const
        {
            return *this;
        }

        /// Create a new par_simd_policy referencing an executor and
        /// a chunk size.
        ///
        /// \param exec         [in] The executor to use for the execution of
        ///                     the parallel algorithm the returned execution
        ///                     policy is used with
        ///
        /// \returns The new par_simd_policy
        ///
        template <typename Executor>
        typename parallel::execution::rebind_executor<par_simd_policy, Executor,
            executor_parameters_type>::type
        on(Executor&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor>::value,
                "pika::traits::is_executor_any<Executor>::value");

            typedef typename parallel::execution::rebind_executor<par_simd_policy, Executor,
                executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor, exec), parameters());
        }

        /// Create a new par_simd_policy from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor_parameters<Parameters>::value is true
        ///
        /// \returns The new par_simd_policy
        ///
        template <typename... Parameters,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters...>::type>
        typename parallel::execution::rebind_executor<par_simd_policy, executor_type,
            ParametersType>::type
        with(Parameters&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<par_simd_policy, executor_type,
                ParametersType>::type rebound_type;
            return rebound_type(executor(),
                parallel::execution::join_executor_parameters(PIKA_FORWARD(Parameters, params)...));
        }

        using base_policy_type = parallel_policy;
        base_policy_type base_policy()
        {
            return parallel_policy{};
        }

    public:
        /// Return the associated executor object.
        executor_type& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr executor_type const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        executor_parameters_type& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr executor_parameters_type const& parameters() const
        {
            return params_;
        }

    private:
        executor_type exec_;
        executor_parameters_type params_;
    };

    /// Default data-parallel execution policy object.
    static constexpr par_simd_policy par_simd{};

    /// The class par_simd_policy_shim is an execution policy type
    /// used as a unique type to disambiguate parallel algorithm overloading
    /// and indicate that a parallel algorithm's execution may be parallelized.
    template <typename Executor, typename Parameters>
    struct par_simd_policy_shim : par_simd_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = Executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        using executor_parameters_type = Parameters;

        /// The category of the execution agents created by this execution
        /// policy.
        typedef typename pika::traits::executor_execution_category<executor_type>::type
            execution_category;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = par_simd_policy_shim<Executor_, Parameters_>;
        };

        /// Create a new par_simd_task_policy_shim
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new par_simd_task_policy_shim
        ///
        constexpr par_simd_task_policy_shim<Executor, Parameters> operator()(task_policy_tag) const
        {
            return par_simd_task_policy_shim<Executor, Parameters>(exec_, params_);
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task par_simd policy
        ///
        constexpr decltype(auto) operator()(non_task_policy_tag /*tag*/) const
        {
            return *this;
        }

        /// Create a new par_simd_task_policy_shim from the given
        /// executor
        ///
        /// \tparam Executor    The type of the executor to associate with this
        ///                     execution policy.
        ///
        /// \param exec         [in] The executor to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor<Executor>::value is true
        ///
        /// \returns The new parallel_policy
        ///
        template <typename Executor_>
        typename parallel::execution::rebind_executor<par_simd_policy_shim, Executor_,
            executor_parameters_type>::type
        on(Executor_&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor_>::value,
                "pika::traits::is_executor_any<Executor_>::value");

            typedef typename parallel::execution::rebind_executor<par_simd_policy_shim, Executor_,
                executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor_, exec), params_);
        }

        /// Create a new par_simd_policy_shim from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor_parameters<Parameters>::value is true
        ///
        /// \returns The new par_simd_policy_shim
        ///
        template <typename... Parameters_,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters_...>::type>
        typename parallel::execution::rebind_executor<par_simd_policy_shim, executor_type,
            ParametersType>::type
        with(Parameters_&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<par_simd_policy_shim,
                executor_type, ParametersType>::type rebound_type;
            return rebound_type(exec_,
                parallel::execution::join_executor_parameters(
                    PIKA_FORWARD(Parameters_, params)...));
        }

        using base_policy_type = parallel_policy_shim<Executor, Parameters>;
        base_policy_type base_policy()
        {
            return parallel_policy_shim<Executor, Parameters>(exec_, params_);
        }

        /// Return the associated executor object.
        Executor& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr Executor const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        Parameters& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr Parameters const& parameters() const
        {
            return params_;
        }

        /// \cond NOINTERNAL
        constexpr par_simd_policy_shim() = default;

        template <typename Executor_, typename Parameters_>
        constexpr par_simd_policy_shim(Executor_&& exec, Parameters_&& params)
          : exec_(PIKA_FORWARD(Executor_, exec))
          , params_(PIKA_FORWARD(Parameters_, params))
        {
        }

    private:
        Executor exec_;
        Parameters params_;
        /// \endcond
    };

    /// The class par_simd_task_policy_shim is an
    /// execution policy type used as a unique type to disambiguate parallel
    /// algorithm overloading based on combining a underlying
    /// \a par_simd_task_policy and an executor and indicate that
    /// a parallel algorithm's execution may be parallelized and vectorized.
    template <typename Executor, typename Parameters>
    struct par_simd_task_policy_shim : par_simd_task_policy
    {
        /// The type of the executor associated with this execution policy
        using executor_type = Executor;

        /// The type of the associated executor parameters object which is
        /// associated with this execution policy
        using executor_parameters_type = Parameters;

        /// The category of the execution agents created by this execution
        /// policy.
        typedef typename pika::traits::executor_execution_category<executor_type>::type
            execution_category;

        /// Rebind the type of executor used by this execution policy. The
        /// execution category of Executor shall not be weaker than that of
        /// this execution policy
        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            /// The type of the rebound execution policy
            using type = par_simd_task_policy_shim<Executor_, Parameters_>;
        };

        /// Create a new par_simd_task_policy_shim from itself
        ///
        /// \param tag          [in] Specify that the corresponding asynchronous
        ///                     execution policy should be used
        ///
        /// \returns The new sequenced_task_policy
        ///
        constexpr par_simd_task_policy_shim operator()(task_policy_tag) const
        {
            return *this;
        }

        /// Create a new non task policy from itself
        ///
        /// \returns The non task par_simd policy
        ///
        inline decltype(auto) operator()(non_task_policy_tag /*tag*/) const;

        /// Create a new parallel_task_policy from the given
        /// executor
        ///
        /// \tparam Executor    The type of the executor to associate with this
        ///                     execution policy.
        ///
        /// \param exec         [in] The executor to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: is_executor<Executor>::value is true
        ///
        /// \returns The new parallel_task_policy
        ///
        template <typename Executor_>
        typename parallel::execution::rebind_executor<par_simd_task_policy_shim, Executor_,
            executor_parameters_type>::type
        on(Executor_&& exec) const
        {
            static_assert(pika::traits::is_executor_any<Executor_>::value,
                "pika::traits::is_executor_any<Executor_>::value");

            typedef typename parallel::execution::rebind_executor<par_simd_task_policy_shim,
                Executor_, executor_parameters_type>::type rebound_type;
            return rebound_type(PIKA_FORWARD(Executor_, exec), params_);
        }

        /// Create a new parallel_policy_shim from the given
        /// execution parameters
        ///
        /// \tparam Parameters  The type of the executor parameters to
        ///                     associate with this execution policy.
        ///
        /// \param params       [in] The executor parameters to use for the
        ///                     execution of the parallel algorithm the
        ///                     returned execution policy is used with.
        ///
        /// \note Requires: all parameters are executor_parameters,
        ///                 different parameter types can't be duplicated
        ///
        /// \returns The new parallel_policy_shim
        ///
        template <typename... Parameters_,
            typename ParametersType =
                typename parallel::execution::executor_parameters_join<Parameters_...>::type>
        typename parallel::execution::rebind_executor<par_simd_task_policy_shim, executor_type,
            ParametersType>::type
        with(Parameters_&&... params) const
        {
            typedef typename parallel::execution::rebind_executor<par_simd_task_policy_shim,
                executor_type, ParametersType>::type rebound_type;
            return rebound_type(exec_,
                parallel::execution::join_executor_parameters(
                    PIKA_FORWARD(Parameters_, params)...));
        }

        using base_policy_type = parallel_task_policy_shim<Executor, Parameters>;
        base_policy_type base_policy()
        {
            return parallel_task_policy_shim<Executor, Parameters>(exec_, params_);
        }

        /// Return the associated executor object.
        Executor& executor()
        {
            return exec_;
        }
        /// Return the associated executor object.
        constexpr Executor const& executor() const
        {
            return exec_;
        }

        /// Return the associated executor parameters object.
        Parameters& parameters()
        {
            return params_;
        }
        /// Return the associated executor parameters object.
        constexpr Parameters const& parameters() const
        {
            return params_;
        }

        /// \cond NOINTERNAL
        constexpr par_simd_task_policy_shim() = default;

        template <typename Executor_, typename Parameters_>
        constexpr par_simd_task_policy_shim(Executor_&& exec, Parameters_&& params)
          : exec_(PIKA_FORWARD(Executor_, exec))
          , params_(PIKA_FORWARD(Parameters_, params))
        {
        }

    private:
        Executor exec_;
        Parameters params_;
        /// \endcond
    };

    decltype(auto) simd_task_policy::operator()(non_task_policy_tag /*tag*/) const
    {
        return simd.on(executor()).with(parameters());
    }

    decltype(auto) par_simd_task_policy::operator()(non_task_policy_tag /*tag*/) const
    {
        return par_simd.on(executor()).with(parameters());
    }

    template <typename Executor, typename Parameters>
    decltype(auto)
    simd_task_policy_shim<Executor, Parameters>::operator()(non_task_policy_tag /*tag*/) const
    {
        return simd_policy_shim<Executor, Parameters>{}.on(executor()).with(parameters());
    }

    template <typename Executor, typename Parameters>
    decltype(auto)
    par_simd_task_policy_shim<Executor, Parameters>::operator()(non_task_policy_tag /*tag*/) const
    {
        return par_simd_policy_shim<Executor, Parameters>{}.on(executor()).with(parameters());
    }
}    // namespace pika::execution

namespace pika::detail {
    ///////////////////////////////////////////////////////////////////////////
    // extensions

    /// \cond NOINTERNAL
    template <>
    struct is_execution_policy<pika::execution::simd_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<pika::execution::simd_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <>
    struct is_execution_policy<pika::execution::simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<pika::execution::simd_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <>
    struct is_execution_policy<pika::execution::par_simd_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<pika::execution::par_simd_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <>
    struct is_execution_policy<pika::execution::par_simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<pika::execution::par_simd_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <>
    struct is_sequenced_execution_policy<pika::execution::simd_policy> : std::true_type
    {
    };

    template <>
    struct is_sequenced_execution_policy<pika::execution::simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_sequenced_execution_policy<pika::execution::simd_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_sequenced_execution_policy<
        pika::execution::simd_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <>
    struct is_async_execution_policy<pika::execution::simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_async_execution_policy<pika::execution::simd_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <>
    struct is_async_execution_policy<pika::execution::par_simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_async_execution_policy<
        pika::execution::par_simd_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <>
    struct is_parallel_execution_policy<pika::execution::par_simd_policy> : std::true_type
    {
    };

    template <>
    struct is_parallel_execution_policy<pika::execution::par_simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_parallel_execution_policy<pika::execution::par_simd_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_parallel_execution_policy<
        pika::execution::par_simd_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <>
    struct is_vectorpack_execution_policy<pika::execution::simd_policy> : std::true_type
    {
    };

    template <>
    struct is_vectorpack_execution_policy<pika::execution::simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_vectorpack_execution_policy<pika::execution::simd_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_vectorpack_execution_policy<
        pika::execution::simd_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <>
    struct is_vectorpack_execution_policy<pika::execution::par_simd_policy> : std::true_type
    {
    };

    template <>
    struct is_vectorpack_execution_policy<pika::execution::par_simd_task_policy> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_vectorpack_execution_policy<
        pika::execution::par_simd_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_vectorpack_execution_policy<
        pika::execution::par_simd_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };
    /// \endcond
}    // namespace pika::detail
