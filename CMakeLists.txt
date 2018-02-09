cmake_minimum_required( VERSION 2.8 )
project( SUNBEAM )
enable_language( CXX )
include( CheckCXXSourceCompiles )
include( CTest )

find_package(opm-parser REQUIRED)
find_package(PythonInterp 2.7 REQUIRED)

set( warnings "-Wall -Wextra -pedantic -Wpointer-arith" )
set( warnings "${warnings} -Wformat-nonliteral -Wcast-align" )
set( warnings "${warnings} -Wmissing-declarations -Wcast-qual" )
set( warnings "${warnings} -Wwrite-strings -Wchar-subscripts " )
set( warnings "${warnings} -Wredundant-decls" )
set( CMAKE_CXX_FLAGS "-std=c++11 ${warnings} ${CMAKE_CXX_FLAGS}" )


list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pycmake/cmake/Modules)

option( USE_RPATH "Embed RPATH in libraries and binaries" OFF)
if (USE_RPATH)
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

add_subdirectory( pybind11 )
add_subdirectory( python )
add_subdirectory( sunbeam )
add_subdirectory( tests )

