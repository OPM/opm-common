#!/bin/bash

source ${TOOLCHAIN_DIR}/build-configurations-sca.sh
source `dirname $0`/build-opm-module.sh

CPPCHECK_IGNORE_LIST="-i$WORKSPACE/tests/material/test_fluidmatrixinteractions.cpp"

$WORKSPACE/jenkins/build.sh

run_static_analysis opm-common
