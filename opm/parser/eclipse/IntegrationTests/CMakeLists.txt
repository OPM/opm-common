add_executable(runIntegrationTests IntegrationTests.cpp)

target_link_libraries(runIntegrationTests Parser ${Boost_LIBRARIES})

add_test(NAME runIntegrationTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runIntegrationTests)

set_property(SOURCE IntegrationTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")
