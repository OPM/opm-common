add_executable(runErtTests test_ert_wrapper.cpp)
target_link_libraries(runErtTests Parser ${Boost_LIBRARIES})
add_test(NAME runErtTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runErtTests )
