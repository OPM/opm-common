#!/bin/bash

source `dirname $0`/build-opm-module.sh

# Create symlink so build_module can find the test result converter
mkdir deps
ln -sf $WORKSPACE deps/opm-common

pushd .
mkdir -p serial/build-opm-common
cd serial/build-opm-common
build_module "-DCMAKE_INSTALL_PREFIX=$WORKSPACE/serial/install" 1 $WORKSPACE
test $? -eq 0 || exit 1
popd

# If no downstream builds we are done
if ! grep -q "with downstreams" <<< $ghprbCommentBody
then
  cp serial/build-opm-common/testoutput.xml .
  exit 0
fi

ERT_REVISION=master

if grep -q "ert=" <<< $ghprbCommentBody
then
  ERT_REVISION=pull/`echo $ghprbCommentBody | sed -r 's/.*ert=([0-9]+).*/\1/g'`/merge
fi

source $WORKSPACE/deps/opm-common/jenkins/setup-opm-data.sh

# Downstream revisions
declare -a downstreams
downstreams=(ert
             opm-parser
             opm-output
             opm-material
             opm-core
             opm-grid
             opm-simulators
             opm-upscaling
             ewoms)

declare -A downstreamRev
downstreamRev[ert]=master
downstreamRev[opm-parser]=master
downstreamRev[opm-material]=master
downstreamRev[opm-core]=master
downstreamRev[opm-grid]=master
downstreamRev[opm-output]=master
downstreamRev[opm-simulators]=master
downstreamRev[opm-upscaling]=master
downstreamRev[ewoms]=master

build_downstreams opm-common

test $? -eq 0 || exit 1
