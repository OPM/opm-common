set(PYTHON_SOURCES
    __init__.py
    sunbeam.py
    config.py
    properties.py
    schedule.py)

add_python_package(sunbeam sunbeam "${PYTHON_SOURCES}")
