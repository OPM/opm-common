add_executable(runDeckRecordTests DeckRecordTests.cpp)
target_link_libraries(runDeckRecordTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckRecordTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckRecordTests  )
set_property(SOURCE DeckRecordTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")


add_executable(runDeckItemTests DeckItemTests.cpp)
target_link_libraries(runDeckItemTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckItemTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckItemTests  )
set_property(SOURCE DeckItemTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")


add_executable(runDeckTests DeckTests.cpp)
target_link_libraries(runDeckTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckTests  )
set_property(SOURCE DeckTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")




