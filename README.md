# sunbeam ![build status](https://travis-ci.org/Statoil/sunbeam.svg?branch=master "TravisCI Build Status")

# Quick start (WIP)
Assumes you have built (and maybe installed) opm-parser
```
git clone --recursive git@github.com:statoil/sunbeam`
cd sunbeam
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=$OPM_COMMON_ROOT/cmake/Modules
make
```
