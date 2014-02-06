add_executable( runjsonTests jsonTests.cpp )
target_link_libraries( runjsonTests opm-json ${Boost_LIBRARIES})

set_source_files_properties( jsonTests.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-variable")
add_test( NAME runjsonTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runjsonTests )
