#!/bin/bash -l

# Copyright (c) 2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Make undefined variables errors, print each command
set -eux

# Clean up old artifacts
rm -rf ./jenkins-pika* ./*-Testing

source ".jenkins/cscs-perftests/slurm-constraint-${configuration_name}.sh"

if [[ -z "${ghprbPullId:-}" ]]; then
    # Set name of branch if not building a pull request
    export git_local_branch=$(echo ${GIT_BRANCH} | cut -f2 -d'/')
    job_name="jenkins-pika-${git_local_branch}-${configuration_name}"
else
    job_name="jenkins-pika-${ghprbPullId}-${configuration_name}"

    # Cancel currently running builds on the same branch, but only for pull
    # requests
    scancel --account="djenkssl" --jobname="${job_name}"
fi

# Start the actual build
set +e
sbatch \
    --job-name="${job_name}" \
    --nodes="1" \
    --constraint="${configuration_slurm_constraint}" \
    --partition="cscsci" \
    --account="djenkssl" \
    --time="00:15:00" \
    --output="jenkins-pika-${configuration_name}.out" \
    --error="jenkins-pika-${configuration_name}.err" \
    --wait .jenkins/cscs-perftests/batch.sh

# Print slurm logs
echo "= stdout =================================================="
cat "jenkins-pika-${configuration_name}.out"

echo "= stderr =================================================="
cat "jenkins-pika-${configuration_name}.err"

# Get build status
status_file="jenkins-pika-${configuration_name}-ctest-status.txt"

# Comment on the PR if any failures
if [[ $(cat ${status_file}) != 0 ]]; then
    ./.jenkins/cscs-perftests/comment_github.sh
fi

set -e
exit $(cat ${status_file})
