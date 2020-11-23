# Dockerfile to generate PyPI packages. Needs to be run from the opm-common root folder
# Example use:
# sudo docker build -t manylinux2014_opm:built . -f python/Dockerfile

FROM quay.io/pypa/manylinux2014_x86_64
RUN yum-config-manager --add-repo https://www.opm-project.org/package/opm.repo
RUN yum install -y cmake3 ccache boost169-devel boost169-static
RUN yum install -y blas-devel suitesparse-devel trilinos-openmpi-devel

RUN /opt/python/cp35-cp35m/bin/python -m pip install pip --upgrade
RUN /opt/python/cp35-cp35m/bin/python -m pip install wheel setuptools twine pytest-runner auditwheel
RUN /opt/python/cp36-cp36m/bin/python -m pip install pip --upgrade
RUN /opt/python/cp36-cp36m/bin/python -m pip install wheel setuptools twine pytest-runner auditwheel
RUN /opt/python/cp37-cp37m/bin/python -m pip install pip --upgrade
RUN /opt/python/cp37-cp37m/bin/python -m pip install wheel setuptools twine pytest-runner auditwheel
RUN /opt/python/cp38-cp38/bin/python -m pip install pip --upgrade
RUN /opt/python/cp38-cp38/bin/python -m pip install wheel setuptools twine pytest-runner auditwheel
RUN /opt/python/cp39-cp39/bin/python -m pip install pip --upgrade
RUN /opt/python/cp39-cp39/bin/python -m pip install wheel setuptools twine pytest-runner auditwheel


ENV version ""
ENV build_dir ""
WORKDIR /src

CMD ["sh", "-c", "python/generate-pypi-package.sh ${build_dir} ${version} > ${build_dir}/build.out 2>&1"]

