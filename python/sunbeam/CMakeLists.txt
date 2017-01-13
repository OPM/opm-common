include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR}
                           ${opm-parser_INCLUDE_DIRS})
add_library( sunbeam SHARED sunbeam.cpp )
target_link_libraries( sunbeam ${Boost_LIBRARIES} ${opm-parser_LIBRARIES} )

set(PYTHON_SOURCES
    __init__.py
    sunbeam.py
    config.py
    properties.py
    schedule.py)

add_python_package(sunbeam sunbeam "${PYTHON_SOURCES}")
