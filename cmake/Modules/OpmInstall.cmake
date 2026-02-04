# - Installation function
#
# Set up installation targets for the binary library. The following
# suffices must be defined for the prefix passed as parameter:
#
# _NAME             Name of the library
# _HEADERS          List of header files to install
# _TARGET           CMake target which builds the library
# _LIBRARY_TYPE     Static or shared library
# _DEBUG            File containing debug symbols
include (GNUInstallDirs)

function(opm_install opm)
  install(
    TARGETS
      ${${opm}_TARGET} ${${opm}_EXTRA_TARGETS}
    EXPORT
      ${opm}-targets
    LIBRARY DESTINATION
      ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION
      ${CMAKE_INSTALL_LIBDIR}
    FILE_SET
      HEADERS
    FILE_SET
      generated_headers
  )
  if(NOT "${${opm}_TARGET}" STREQUAL "")
    get_target_property(lib_type ${${opm}_TARGET} TYPE)
    if(lib_type MATCHES "INTERFACE" AND NOT ${opm}_EXTRA_TARGETS)
      set(cmake_config_dir ${CMAKE_INSTALL_DATADIR}/cmake/${${opm}_NAME})
    else()
      set(cmake_config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/${${opm}_NAME})
    endif()

    export(
      TARGETS
        ${${opm}_TARGET}
        ${${opm}_EXTRA_TARGETS}
      FILE
        ${opm}-targets.cmake
    )
    install(
      EXPORT
        ${opm}-targets
      DESTINATION
        ${cmake_config_dir}
    )
  endif()

  # note that the DUNE parts that looks for dune.module is currently (2013-09) not
  # multiarch-aware and will thus put in lib64/ on RHEL and lib/ on Debian
  install(
    FILES
      ${PROJECT_SOURCE_DIR}/dune.module
    DESTINATION
      lib/dunecontrol/${${opm}_NAME}
  )
  install(
    FILES
      ${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}-prereqs.cmake
    DESTINATION
      ${CMAKE_INSTALL_DATADIR}/opm/cmake/Modules
  )
endfunction()
