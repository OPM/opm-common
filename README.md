# sunbeam [![Build Status](https://travis-ci.org/equinor/sunbeam.svg?branch=master)](https://travis-ci.org/equinor/sunbeam)


# Prerequisites:

Clone and build `ecl` from [Statoil/libecl](https://github.com/Statoil/libecl)
and `opm-commonr` from [OPM/opm-common](https://github.com/OPM/opm-common).

```
git clone https://github.com/Statoil/libecl
git clone https://github.com/OPM/opm-parser
mkdir libecl/build
mkdir opm-common/build
pushd libecl/build
cmake ..
make install
popd
pushd opm-common/build
cmake ..
make install
```

# Quick start (WIP)
Assumes you have built (and maybe installed) opm-common

```bash
git clone --recursive https://github.com/Statoil/sunbeam
mkdir sunbeam/build
pushd sunbeam/build
cmake ..
make
ctest # for running unit tests
```

# Obtaining test data

```bash
cd ~
git clone https://github.com/OPM/opm-data
cd opm-data/norne
```

With `opm-data` available, one can test sunbeam on Norne:

```python
import sunbeam
es = sunbeam.parse('NORNE_ATW2013.DATA', ('PARSE_RANDOM_SLASH', sunbeam.action.ignore))
len(es.faults())
```

Remember to have `~/sunbeam/build/python` in your `$PYTHONPATH`.
