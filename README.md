# sunbeam ![build status](https://travis-ci.org/Statoil/sunbeam.svg?branch=master "TravisCI Build Status")

# Quick start (WIP)
Assumes you have built (and maybe installed) opm-parser

```bash
git clone --recursive https://github.com/statoil/sunbeam
cd sunbeam
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=$OPM_COMMON_ROOT/cmake/Modules
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
