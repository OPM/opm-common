cmake_minimum_required( VERSION 2.8 )

set(MODULES_INSTALL_PREFIX  "share/opm" CACHE STRING "The modules will be installed in: \${CMAKE_INSTALL_PREFIX}/\${MODULES_INSTALL_PREFIX}/cmake/Modules")

install(DIRECTORY cmake/Modules DESTINATION ${CMAKE_INSTALL_PREFIX}/${MODULES_INSTALL_PREFIX}/cmake)
install(DIRECTORY cmake/Scripts DESTINATION ${CMAKE_INSTALL_PREFIX}/${MODULES_INSTALL_PREFIX}/cmake)
install(DIRECTORY cmake/Templates DESTINATION ${CMAKE_INSTALL_PREFIX}/${MODULES_INSTALL_PREFIX}/cmake)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
include(OpmInit)

file (READ ${PROJECT_SOURCE_DIR}/dune.module DUNE_MODULE)
OpmGetDuneModuleDirective ("Version" opm-common_VERSION "${DUNE_MODULE}")

configure_file(opm-common-config.cmake.in opm-common-config.cmake @ONLY)
install(FILES ${PROJECT_BINARY_DIR}/opm-common-config.cmake
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/opm-common)
