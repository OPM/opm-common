add_executable(runTimeMapTests TimeMapTest.cpp)
target_link_libraries(runTimeMapTests Parser ${Boost_LIBRARIES})
add_test(NAME runTimeMapTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runTimeMapTests )


add_executable(runScheduleTests ScheduleTests.cpp)
target_link_libraries(runScheduleTests Parser ${Boost_LIBRARIES})
add_test(NAME runScheduleTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runScheduleTests )


add_executable(runWellTests WellTests.cpp)
target_link_libraries(runWellTests Parser ${Boost_LIBRARIES})
add_test(NAME runWellTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runWellTests )


add_executable(runWellPropertiesTests WellPropertiesTests.cpp)
target_link_libraries(runWellPropertiesTests Parser ${Boost_LIBRARIES})
add_test(NAME runWellPropertiesTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runWellPropertiesTests )


add_executable(runWellSetTests WellSetTests.cpp)
target_link_libraries(runWellSetTests Parser ${Boost_LIBRARIES})
add_test(NAME runWellSetTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runWellSetTests )


add_executable(runGroupTests GroupTests.cpp)
target_link_libraries(runGroupTests Parser ${Boost_LIBRARIES})
add_test(NAME runGroupTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runGroupTests )


add_executable(runOrderedMapTests OrderedMapTests.cpp)
target_link_libraries(runOrderedMapTests Parser ${Boost_LIBRARIES})
add_test(NAME runOrderedMapTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runOrderedMapTests )


add_executable(runScheduleEnumTests ScheduleEnumTests.cpp)
target_link_libraries(runScheduleEnumTests Parser ${Boost_LIBRARIES})
add_test(NAME runScheduleEnumTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runScheduleEnumTests )


add_executable(runCompletionTests CompletionTests.cpp)
target_link_libraries(runCompletionTests Parser ${Boost_LIBRARIES})
add_test(NAME runCompletionTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runCompletionTests )


add_executable(runCompletionSetTests CompletionSetTests.cpp)
target_link_libraries(runCompletionSetTests Parser ${Boost_LIBRARIES})
add_test(NAME runCompletionSetTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runCompletionSetTests )


add_executable(runDynamicStateTests DynamicStateTests.cpp)
target_link_libraries(runDynamicStateTests Parser ${Boost_LIBRARIES})
add_test(NAME runDynamicStateTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDynamicStateTests )


add_executable(runGroupTreeNodeTests GroupTreeNodeTests.cpp)
target_link_libraries(runGroupTreeNodeTests Parser ${Boost_LIBRARIES})
add_test(NAME runGroupTreeNodeTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runGroupTreeNodeTests )


add_executable(runGroupTreeTests GroupTreeTests.cpp)
target_link_libraries(runGroupTreeTests Parser ${Boost_LIBRARIES})
add_test(NAME runGroupTreeTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runGroupTreeTests )
