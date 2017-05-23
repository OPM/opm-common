set (TEST_SOURCES
    __init__.py
    test_grid.py
    test_props.py )

add_python_package("python.tests.ecl_state.grid" "${PYTHON_INSTALL_PREFIX}/tests/ecl_state/grid" "${TEST_SOURCES}" False)

addPythontest( python.test_grid tests.ecl_state.grid.test_grid.EclipseGridTest )
addPythontest( python.test_props tests.ecl_state.grid.test_props.Eclipse3DPropsTest )
