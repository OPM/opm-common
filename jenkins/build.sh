#!/bin/bash

# No upstreams
declare -a upstreams
declare -A upstreamRev

# Downstreams and revisions
declare -a downstreams
downstreams=(opm-grid
             opm-simulators
             opm-upscaling
             )

declare -A downstreamRev
downstreamRev[opm-grid]=master
downstreamRev[opm-simulators]=master
downstreamRev[opm-upscaling]=master

source `dirname $0`/build-opm-module.sh

parseRevisions
printHeader opm-common

clone_repositories opm-common

# Setup opm-tests
source $WORKSPACE/deps/opm-common/jenkins/setup-opm-tests.sh

build_module_full opm-common
