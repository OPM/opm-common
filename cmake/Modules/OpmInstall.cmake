# - Installation macro
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

macro (opm_install opm)
  install(
    TARGETS
      ${${opm}_TARGET} ${${opm}_EXTRA_TARGETS}
    EXPORT
      ${opm}-targets
    LIBRARY DESTINATION
      ${CMAKE_INSTALL_LIBDIR}${${opm}_VER_DIR}
    ARCHIVE DESTINATION
      ${CMAKE_INSTALL_LIBDIR}${${opm}_VER_DIR}
    FILE_SET
      HEADERS
    FILE_SET
      generated_headers
  )
  if(NOT "${${opm}_TARGET}" STREQUAL "")
    export(TARGETS ${${opm}_TARGET} ${${opm}_EXTRA_TARGETS}
            FILE ${opm}-targets.cmake)
    install(EXPORT ${opm}-targets DESTINATION "share/cmake/${opm}")
  endif()

  # note that the DUNE parts that looks for dune.module is currently (2013-09) not
  # multiarch-aware and will thus put in lib64/ on RHEL and lib/ on Debian
  install (
	FILES ${PROJECT_SOURCE_DIR}/dune.module
	DESTINATION lib/${${opm}_VER_DIR}/dunecontrol/${${opm}_NAME}
	)
  install (
        FILES ${PROJECT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}-prereqs.cmake
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/opm/cmake/Modules
        )
endmacro (opm_install opm)
