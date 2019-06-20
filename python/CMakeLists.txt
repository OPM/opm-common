
find_package(PythonInterp REQUIRED)

set( warnings "-Wall -Wextra -pedantic -Wpointer-arith" )
set( warnings "${warnings} -Wformat-nonliteral -Wcast-align" )
set( warnings "${warnings} -Wmissing-declarations -Wcast-qual" )
set( warnings "${warnings} -Wwrite-strings -Wchar-subscripts " )
set( warnings "${warnings} -Wredundant-decls" )
set( CMAKE_CXX_FLAGS "-std=c++11 ${warnings} ${CMAKE_CXX_FLAGS}" )


list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/python/pycmake)

option( USE_RPATH "Embed RPATH in libraries and binaries" OFF)
if (USE_RPATH)
    SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif ()

add_subdirectory( pybind11 )
add_subdirectory( python )
add_subdirectory( sunbeam )
add_subdirectory( tests )

