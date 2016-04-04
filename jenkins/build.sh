#!/bin/bash

source `dirname $0`/build-opm-module.sh

# Create symlink so build_module can find the test result converter
mkdir deps
ln -sf $WORKSPACE deps/opm-common

# Build without MPI
pushd .
mkdir -p serial/build-opm-common
cd serial/build-opm-common
build_module "-DUSE_MPI=0" 1 $WORKSPACE
test $? -eq 0 || exit 1
popd

cp serial/build-opm-common/testoutput.xml .
