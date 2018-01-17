#!/bin/bash

source `dirname $0`/build-opm-module.sh

# Create symlink so build_module can find the test result converter
mkdir deps
ln -sf $WORKSPACE deps/opm-common

# Downstreams and revisions
declare -a downstreams
downstreams=(libecl
             opm-parser
             opm-output
             opm-material
             opm-grid
             ewoms
             opm-simulators
             opm-upscaling
             )

declare -A downstreamRev
downstreamRev[libecl]=master
downstreamRev[opm-parser]=master
downstreamRev[opm-material]=master
downstreamRev[opm-grid]=master
downstreamRev[opm-output]=master
downstreamRev[ewoms]=master
downstreamRev[opm-simulators]=master
downstreamRev[opm-upscaling]=master

parseRevisions
printHeader opm-common

# Setup opm-data if necessary
if grep -q "with downstreams" <<< $ghprbCommentBody
then
  source $WORKSPACE/deps/opm-common/jenkins/setup-opm-data.sh
fi

build_module_full opm-common
