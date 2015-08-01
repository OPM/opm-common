set(PYTHON_SOURCES
    __init__.py
)

add_python_package("opm"  ${PYTHON_INSTALL_PREFIX}/opm "${PYTHON_SOURCES}" True)

add_subdirectory( parser )
add_subdirectory( deck )
add_subdirectory( ecl_state )
add_subdirectory( log )


configure_file(opm_lib_info_build.py.in   ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}/opm/__opm_lib_info.py )
configure_file(opm_lib_info_install.py.in ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}/opm_lib_info_install.py )
install(FILES ${PROJECT_BINARY_DIR}/${PYTHON_INSTALL_PREFIX}/opm_lib_info_install.py DESTINATION ${PYTHON_INSTALL_PREFIX}/opm RENAME __opm_lib_info.py)
