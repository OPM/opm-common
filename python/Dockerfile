# Dockerfile to generate PyPI packages. Needs to be run from the opm-common root folder
# Example use:
# sudo docker build -t manylinux2014_opm:built . -f python/Dockerfile

FROM quay.io/pypa/manylinux2014_x86_64 AS stage1
ARG version_tag=""
WORKDIR /tmp/opm-common
RUN echo "Using package version tag: $version_tag"
ADD . .
RUN /bin/bash /tmp/opm-common/python/generate-pypi-package.sh $version_tag

FROM scratch AS export_stage
COPY --from=stage1 /tmp/opm-common/wheelhouse .
