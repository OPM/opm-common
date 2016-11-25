include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR}
                           ${opm-parser_INCLUDE_DIRS})
add_library( sunbeam SHARED sunbeam.cpp )
target_link_libraries( sunbeam ${Boost_LIBRARIES} ${opm-parser_LIBRARIES} )
