#!/usr/bin/env bash
set -e

pushd . > /dev/null
opm-parser/travis/build-opm-parser.sh
cd opm-parser/build
ctest --output-on-failure
popd > /dev/null
