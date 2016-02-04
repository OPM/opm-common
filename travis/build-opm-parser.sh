#!/usr/bin/env bash
set -e

pushd . > /dev/null
cd opm-parser
mkdir build
cd build
cmake -DENABLE_PYTHON=ON ../
make
popd > /dev/null
