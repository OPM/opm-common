add_executable(runIntegrationTests IntegrationTests.cpp)
target_link_libraries(runIntegrationTests Parser ${Boost_LIBRARIES})
add_test(NAME runIntegrationTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runIntegrationTests)
set_property(SOURCE IntegrationTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")

add_executable(runIntegrationTestsInternalData IntegrationTestsInternalData.cpp)
target_link_libraries(runIntegrationTestsInternalData Parser ${Boost_LIBRARIES})
add_test(NAME runIntegrationTestsInternalData WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runIntegrationTestsInternalData )
set_property(TEST runIntegrationTestsInternalData PROPERTY LABELS Statoil)
