add_executable(runOpmLogTests OpmLogTests.cpp)
target_link_libraries(runOpmLogTests Parser  ${Boost_LIBRARIES})
add_test(NAME runOpmLogTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runOpmLogTests  )
