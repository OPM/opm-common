#!/bin/bash

# Needs to be done beforehand:
# docker run -e PLAT=manylinux2014_x86_64 -it quay.io/pypa/manylinux2014_x86_64
#cd /tmp
#git clone https://github.com/CeetronSolutions/opm-common.git
#cd opm-common
#git checkout system-python-pypi

yum-config-manager --add-repo \
https://www.opm-project.org/package/opm.repo
yum install -y boost169-devel boost169-static
yum install -y blas-devel
yum install -y suitesparse-devel
yum install -y trilionos-openmpi-devel
yum install -y cmake3
yum install -y ccache
yum install -y python3-devel
python3 -m pip install pip --upgrade
python3 -m pip install wheel pytest-runner
python3 -m pip install twine
mkdir build && cd build
cmake3 -DPYTHON_EXECUTABLE=/usr/bin/python3 -DBOOST_INCLUDEDIR=/usr/include/boost169 -DBOOST_LIBRARYDIR=/usr/lib64/boost169 \
-DOPM_ENABLE_PYTHON=ON -DOPM_ENABLE_DYNAMIC_BOOST=OFF -DOPM_ENABLE_DYNAMIC_PYTHON_LINKING=OFF ..
# make step is necessary until the generated ParserKeywords/*.hpp
# are generated in the Python step
make -j2
./setup-package.sh
