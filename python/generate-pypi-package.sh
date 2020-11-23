#!/bin/bash

BUILD_DIR=$1
VERSION_TAG=${2:-""}

export PYTHON35=/opt/python/cp35-cp35m/bin/python
export PYTHON36=/opt/python/cp36-cp36m/bin/python
export PYTHON37=/opt/python/cp37-cp37m/bin/python
export PYTHON38=/opt/python/cp38-cp38/bin/python
export PYTHON39=/opt/python/cp39-cp39/bin/python

pushd ${BUILD_DIR}

# make step is necessary until the generated ParserKeywords/*.hpp are generated in the Python step
cmake3 -DBOOST_INCLUDEDIR=/usr/include/boost169 \
       -DBOOST_LIBRARYDIR=/usr/lib64/boost169 \
       -DBUILD_TESTING=OFF ..
make -j 16


cmake3 -DPYTHON_EXECUTABLE=${PYTHON35} \
       -DOPM_ENABLE_PYTHON=ON \
       -DOPM_PYTHON_PACKAGE_VERSION_TAG=${VERSION_TAG} ..
./setup-package.sh
${PYTHON35} -m auditwheel repair python/dist/*cp35*.whl

cmake3 -DPYTHON_EXECUTABLE=${PYTHON36} ..
./setup-package.sh
${PYTHON36} -m auditwheel repair python/dist/*cp36*.whl

cmake3 -DPYTHON_EXECUTABLE=${PYTHON37} ..
./setup-package.sh
${PYTHON37} -m auditwheel repair python/dist/*cp37*.whl

cmake3 -DPYTHON_EXECUTABLE=${PYTHON38} ..
./setup-package.sh
${PYTHON38} -m auditwheel repair python/dist/*cp38*.whl

cmake3 -DPYTHON_EXECUTABLE=${PYTHON39} ..
./setup-package.sh
${PYTHON39} -m auditwheel repair python/dist/*cp39*.whl

# Example of upload
# /usr/bin/python3 -m twine upload --repository testpypi wheelhouse/*

popd
