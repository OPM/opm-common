set (TEST_SOURCES
    __init__.py
    test_opm_log.py)

add_python_package("python.tests.log" "${PYTHON_INSTALL_PREFIX}/tests/opm_log" "${TEST_SOURCES}" False)
addPythontest( python.test_log tests.opm_log.test_opm_log.OpmLogTest )
