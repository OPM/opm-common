set(PYTHON_SOURCES
    __init__.py
    table_manager.py
)

add_python_package("opm.ecl_state"  ${PYTHON_INSTALL_PREFIX}/opm/ecl_state "${PYTHON_SOURCES}" True)
