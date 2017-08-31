include( FindPythonModule )
include( PythonPackage )

include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS}
                           ${Boost_INCLUDE_DIR}
                           ${opm-parser_INCLUDE_DIRS})

add_library( sunbeam SHARED sunbeam.cpp )
set_target_properties( sunbeam PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/python/sunbeam )
target_link_libraries( sunbeam ${Boost_LIBRARIES} ${opm-parser_LIBRARIES} )

install(TARGETS sunbeam DESTINATION ${PYTHON_INSTALL_PREFIX}/sunbeam)
