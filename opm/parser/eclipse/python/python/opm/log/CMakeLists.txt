set(PYTHON_SOURCES
    __init__.py
    message_type.py
    opm_log.py
)

add_python_package("opm.log"  ${PYTHON_INSTALL_PREFIX}/opm/log "${PYTHON_SOURCES}" True)
