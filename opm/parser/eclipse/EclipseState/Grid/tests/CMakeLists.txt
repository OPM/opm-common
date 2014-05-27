add_executable(runEclipseGridTests EclipseGridTests.cpp)
target_link_libraries(runEclipseGridTests Parser ${Boost_LIBRARIES})
add_test(NAME runEclipseGridTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runEclipseGridTests )


add_executable(runGridPropertyTests GridPropertyTests.cpp)
target_link_libraries(runGridPropertyTests Parser ${Boost_LIBRARIES})
add_test(NAME runGridPropertyTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runGridPropertyTests )


add_executable(runGridPropertiesTests GridPropertiesTests.cpp)
target_link_libraries(runGridPropertiesTests Parser ${Boost_LIBRARIES})
add_test(NAME runGridPropertiesTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runGridPropertiesTests )


add_executable(runBoxTests BoxTests.cpp)
target_link_libraries(runBoxTests Parser ${Boost_LIBRARIES})
add_test(NAME runBoxTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runBoxTests )


add_executable(runBoxManagerTests BoxManagerTests.cpp)
target_link_libraries(runBoxManagerTests Parser ${Boost_LIBRARIES})
add_test(NAME runBoxManagerTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runBoxManagerTests )

