add_executable(runTableTests TableTests.cpp)
target_link_libraries(runTableTests Parser ${Boost_LIBRARIES})
add_test(NAME runTableTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runTableTests )

add_executable(runTabdimsTests TabdimsTests.cpp)
target_link_libraries(runTabdimsTests Parser ${Boost_LIBRARIES})
add_test(NAME runTabdimsTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runTabdimsTests )
