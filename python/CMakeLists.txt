cmake_minimum_required( VERSION 2.8 )
project(SUNBEAM C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/pycmake)

find_package(opm-common REQUIRED)
find_package(pybind11 REQUIRED)
find_package(PythonInterp REQUIRED)

add_subdirectory( python )
add_subdirectory( sunbeam )
add_subdirectory( tests )
