cmake_minimum_required(VERSION 2.8.12)
project(eclOutput)

# select Python version 

option(PYTHON27 "Compile library for Python version 2.7" OFF)
option(PYTHON36 "Compile library for Python version 3.6" ON)


if( PYTHON36 )
   set(PYBIND11_PYTHON_VERSION 3.6)
elseif( PYTHON27 )
   set(PYBIND11_PYTHON_VERSION 2.7)
else()
   message( FATAL_ERROR "Available version of Python is 2.7 and 3.6, one of these has to be ON" )   
endif()


# or specify on command line with cmake
# cmake -DPYBIND11_PYTHON_VERSION=3.5 ..

#add_subdirectory(pybind11)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11)

include_directories(../..)


pybind11_add_module(eclfile src/eclfile_binding.cpp)
pybind11_add_module(egrid src/egrid_binding.cpp)
pybind11_add_module(erst src/erst_binding.cpp)
pybind11_add_module(erft src/erft_binding.cpp)
pybind11_add_module(esmry src/esmry_binding.cpp)
pybind11_add_module(eminit src/eminit_binding.cpp)
pybind11_add_module(eclwrite src/eclwrite_binding.cpp)

foreach(module eclfile egrid erst erft esmry eminit eclwrite)

   if( PYTHON36 )
       set_target_properties( ${module} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../shared_obj_files/python3.6 )
   elseif( PYTHON27 )
       set_target_properties( ${module} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/../shared_obj_files/python2.7 )
   endif()

endforeach()


