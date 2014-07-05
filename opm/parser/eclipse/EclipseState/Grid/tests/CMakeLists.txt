add_executable(runEclipseGridTests EclipseGridTests.cpp)
target_link_libraries(runEclipseGridTests Parser ${Boost_LIBRARIES})
add_test(NAME runEclipseGridTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runEclipseGridTests )


add_executable(runGridPropertyTests GridPropertyTests.cpp)
target_link_libraries(runGridPropertyTests Parser ${Boost_LIBRARIES})
add_test(NAME runGridPropertyTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runGridPropertyTests )

add_executable(runFaceDirTests FaceDirTests.cpp)
target_link_libraries(runFaceDirTests Parser ${Boost_LIBRARIES})
add_test(NAME runFaceDirTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runFaceDirTests )


add_executable(runGridPropertiesTests GridPropertiesTests.cpp)
target_link_libraries(runGridPropertiesTests Parser ${Boost_LIBRARIES})
add_test(NAME runGridPropertiesTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runGridPropertiesTests )


add_executable(runBoxTests BoxTests.cpp)
target_link_libraries(runBoxTests Parser ${Boost_LIBRARIES})
add_test(NAME runBoxTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runBoxTests )


add_executable(runBoxManagerTests BoxManagerTests.cpp)
target_link_libraries(runBoxManagerTests Parser ${Boost_LIBRARIES})
add_test(NAME runBoxManagerTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runBoxManagerTests )


add_executable(runTransMultTests TransMultTests.cpp)
target_link_libraries(runTransMultTests Parser ${Boost_LIBRARIES})
add_test(NAME runTransMultTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runTransMultTests )


add_executable(runFaultTests FaultTests.cpp)
target_link_libraries(runFaultTests Parser ${Boost_LIBRARIES})
add_test(NAME runFaultTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runFaultTests )

