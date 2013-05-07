add_executable(runDeckItemTests DeckItemTests.cpp)

target_link_libraries(runDeckItemTests Parser Logger ${Boost_LIBRARIES})

add_test(NAME runDeckItemTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckItemTests  )

set_property(SOURCE DeckItemTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")
