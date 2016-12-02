cmake_minimum_required( VERSION 2.8 )
project( SUNBEAM )
enable_language( CXX )
include( CheckCXXSourceCompiles )
include( CTest )

# assuming gcc-compatible options
set( warnings "-Wall -Wextra -pedantic -Wpointer-arith" )
set( warnings "${warnings} -Wformat-nonliteral -Wcast-align" )
set( warnings "${warnings} -Wmissing-declarations -Wcast-qual" )
set( warnings "${warnings} -Wwrite-strings -Wchar-subscripts " )
set( warnings "${warnings} -Wredundant-decls" )
set( CMAKE_CXX_FLAGS "-std=c++11 ${warnings} ${CMAKE_CXX_FLAGS}" )

find_package( opm-parser REQUIRED )
find_package( PythonLibs 2.7 REQUIRED )
find_package( PythonInterp 2.7 REQUIRED )
find_package( Boost COMPONENTS python REQUIRED )

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/pycmake/cmake/Modules)

add_subdirectory( python )
add_subdirectory( tests )
