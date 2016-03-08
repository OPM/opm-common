#!/usr/bin/env bash
set -e

pushd . > /dev/null
opm-output/travis/build-opm-output.sh
cd opm-output/build
ctest --output-on-failure
popd > /dev/null
