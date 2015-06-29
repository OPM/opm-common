####################################################################
#                                                                  #
# Setup static targets for all submodules.                         #
# Useful when building a static benchmark executable               #
#                                                                  #
####################################################################

# Macros
function(opm_from_git repo name revision)
  externalproject_add(${name}-static
                      GIT_REPOSITORY ${repo}
                      PREFIX static/${name}
                      GIT_TAG ${revision}
                      CONFIGURE_COMMAND PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/static/installed/lib/pkgconfig:${CMAKE_BINARY_DIR}/static/installed/${CMAKE_INSTALL_LIBDIR}/pkgconfig:$ENV{PKG_CONFIG_PATH}
                                 ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/static/installed
                                 -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                 -DBUILD_SHARED_LIBS=0
                                 -DBUILD_TESTING=0 <SOURCE_DIR>)
  set_target_properties(${name}-static PROPERTIES EXCLUDE_FROM_ALL 1)
endfunction()

macro(opm_static_add_dependencies target)
  foreach(arg ${ARGN})
    add_dependencies(${target}-static ${arg}-static)
  endforeach()
endmacro()

include(ExternalProject)
include(UseMultiArch)

# Defaults to building master
if(NOT OPM_BENCHMARK_VERSION)
  set(OPM_BENCHMARK_VERSION "master")
endif()

# ERT
externalproject_add(ert-static
                    GIT_REPOSITORY https://github.com/Ensembles/ert
                    PREFIX static/ert
                    GIT_TAG ${revision}
                    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                                      -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/static/installed
                                      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                                      -DBUILD_SHARED_LIBS=0 <SOURCE_DIR>/devel)
set_target_properties(ert-static PROPERTIES EXCLUDE_FROM_ALL 1)

# 2015.04 release used dune v2.3.1
if(OPM_BENCHMARK_VERSION STREQUAL "release/2015.04/final")
  set(DUNE_VERSION v2.3.1)
endif()

# Master currently uses dune v2.3.1
if(OPM_BENCHMARK_VERSION STREQUAL "master")
  set(DUNE_VERSION v2.3.1)
endif()

# Dune
foreach(dune_repo dune-common dune-geometry dune-grid dune-istl)
  opm_from_git(http://git.dune-project.org/repositories/${dune_repo} ${dune_repo} ${DUNE_VERSION})
endforeach()
opm_static_add_dependencies(dune-istl dune-common)
opm_static_add_dependencies(dune-geometry dune-common)
opm_static_add_dependencies(dune-grid dune-geometry)

# OPM
foreach(opm_repo opm-cmake opm-parser opm-core dune-cornerpoint opm-material
                 opm-porsol opm-upscaling)
  opm_from_git(https://github.com/OPM/${opm_repo} ${opm_repo} ${OPM_BENCHMARK_VERSION})
endforeach()
opm_static_add_dependencies(opm-parser opm-cmake ert)
opm_static_add_dependencies(opm-core opm-parser dune-istl)
opm_static_add_dependencies(dune-cornerpoint opm-core dune-grid)
opm_static_add_dependencies(opm-material opm-core)
opm_static_add_dependencies(opm-porsol dune-cornerpoint opm-material)
opm_static_add_dependencies(opm-upscaling opm-porsol)
