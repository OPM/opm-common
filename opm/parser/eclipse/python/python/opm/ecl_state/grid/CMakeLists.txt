set(PYTHON_SOURCES
    __init__.py
    eclipse_grid.py
    ecl_props.py
)

add_python_package("opm.ecl_state.grid"  ${PYTHON_INSTALL_PREFIX}/opm/ecl_state/grid "${PYTHON_SOURCES}" True)
