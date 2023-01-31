#!/bin/bash

# Run in docker container
# docker run -e PLAT=manylinux2014_x86_64 -it quay.io/pypa/manylinux2014_x86_64

VERSION_TAG=${1:-""}
BUILD_JOBS=4

declare -A python_versions
python_versions[cp36-cp36m]=/opt/python/cp36-cp36m/bin/python
python_versions[cp37-cp37m]=/opt/python/cp37-cp37m/bin/python
python_versions[cp38-cp38]=/opt/python/cp38-cp38/bin/python
python_versions[cp39-cp39]=/opt/python/cp39-cp39/bin/python
python_versions[cp310-cp310]=/opt/python/cp310-cp310/bin/python

source /tmp/opm-common/python/setup-docker-image.sh

cd /tmp/opm-common
mkdir -p /tmp/opm-common/wheelhouse

for tag in ${!python_versions[@]}
do
    # Delete the folder if it already exists
    if [ -d $tag ]; then
      rm -rf $tag
    fi
    mkdir $tag && pushd $tag
    cmake3 -DPYTHON_EXECUTABLE=${python_versions[$tag]} -DBOOST_INCLUDEDIR=/usr/include/boost169 -DBOOST_LIBRARYDIR=/usr/lib64/boost169 -DWITH_NATIVE=0 \
    -DOPM_ENABLE_PYTHON=ON -DOPM_PYTHON_PACKAGE_VERSION_TAG=${VERSION_TAG} ..

    # make step is necessary until the generated ParserKeywords/*.hpp are generated in the Python step
    make opmcommon_python -j${BUILD_JOBS}
    cd python
    ${python_versions[$tag]} setup.py sdist bdist_wheel --plat-name manylinux2014_x86_64 --python-tag $tag
    ${python_versions[$tag]} -m auditwheel repair dist/*$tag*.whl
    cp dist/*$tag*.whl /tmp/opm-common/wheelhouse
    popd
done

# Example of upload
# /usr/bin/python3 -m twine upload --repository testpypi wheelhouse/*
