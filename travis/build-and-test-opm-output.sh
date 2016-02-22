#!/usr/bin/env bash
set -e

pushd . > /dev/null
opm-output/travis/build-opmoutput-
cd opm-core/build
ctest --output-on-failure
popd > /dev/null
