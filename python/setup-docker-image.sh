#!/bin/bash

# Script to be run on a manylinux2014 docker image to complete it for OPM usage.
# i.e. docker run -i -t quay.io/pypa/manylinux2014_x86_64 < setup-docker-image.sh

# A ready made Docker image is available at Dockerhub:
# docker run -i -t lindkvis/manylinux2014_opm:latest

yum-config-manager --add-repo \
https://www.opm-project.org/package/opm.repo
yum install -y cmake3 ccache boost169-devel boost169-static tbb-devel
yum install -y blas-devel suitesparse-devel dune-common-devel

for python_bin in ${python_versions[*]}
do
  ${python_bin} -m pip install pip --upgrade
  ${python_bin} -m pip install wheel setuptools twine pytest-runner auditwheel
done
