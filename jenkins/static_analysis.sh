#!/bin/bash

source ${TOOLCHAIN_DIR}/build-configurations-sca.sh
source `dirname $0`/build-opm-module.sh

$WORKSPACE/jenkins/build.sh

run_static_analysis opm-common
