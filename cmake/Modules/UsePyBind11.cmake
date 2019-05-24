# Downloads the PyBind11 library
#

Include(ExternalProject)
externalproject_add(PyBind11
                    URL https://github.com/pybind/pybind11/archive/v2.2.4.tar.gz
                    CMAKE_ARGS -DPYBIND11_TEST=0 -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/PyBind11-prefix)
set_target_properties(PyBind11 PROPERTIES EXCLUDE_FROM_ALL 1)
