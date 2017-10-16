# sunbeam [![Build Status](https://travis-ci.org/Statoil/sunbeam.svg?branch=master)](https://travis-ci.org/Statoil/sunbeam)


# Prerequisites:

Clone and build `ecl` from [Statoil/libecl](https://github.com/Statoil/libecl)
and `opm-parser` from [OPM/opm-parser](https://github.com/OPM/opm-parser).

```
git clone https://github.com/Statoil/libecl
git clone https://github.com/OPM/opm-parser
mkdir libecl/build
mkdir opm-parser/build
pushd libecl/build
cmake ..
make install
popd
pushd opm-parser/build
cmake ..
make install
```

# Use the correct version of OPM-Parser:

As of October 2017 the master branch of opm-parser is not compatible with
sunbeam. To build sunbeam you should check out and build the branch
`sunbeam-2017.10` of opm-parser:
```
git clone git@github.com:OPM/opm-parser
git checkout sunbeam-2017.10
...
...
```


# Quick start (WIP)
Assumes you have built (and maybe installed) opm-parser

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
