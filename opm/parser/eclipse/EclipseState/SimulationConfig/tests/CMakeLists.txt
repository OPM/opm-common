add_executable(runThresholdPressureTests ThresholdPressureTest.cpp)
target_link_libraries(runThresholdPressureTests Parser ${Boost_LIBRARIES})
add_test(NAME runThresholdPressureTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runThresholdPressureTests )


add_executable(runSimulationConfigTests SimulationConfigTest.cpp)
target_link_libraries(runSimulationConfigTests Parser ${Boost_LIBRARIES})
add_test(NAME runSimulationConfigTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runSimulationConfigTests )
