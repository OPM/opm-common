set (TEST_SOURCES
    __init__.py
    test_parse_mode.py)

add_python_package("python.tests.parse_mode" "${PYTHON_INSTALL_PREFIX}/tests/parse_mode" "${TEST_SOURCES}" False)

addPythontest( python.test_parse_mode tests.parse_mode.test_parse_mode.ParseContextTest )
