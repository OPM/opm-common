add_executable(runDeckRecordTests DeckRecordTests.cpp)
target_link_libraries(runDeckRecordTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckRecordTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckRecordTests  )

add_executable(runDeckIntItemTests DeckIntItemTests.cpp)
target_link_libraries(runDeckIntItemTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckIntItemTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckIntItemTests  )

add_executable(runDeckDoubleItemTests DeckDoubleItemTests.cpp)
target_link_libraries(runDeckDoubleItemTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckDoubleItemTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckDoubleItemTests  )

add_executable(runDeckStringItemTests DeckStringItemTests.cpp)
target_link_libraries(runDeckStringItemTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckStringItemTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckStringItemTests  )

add_executable(runDeckTests DeckTests.cpp)
target_link_libraries(runDeckTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckTests  )

add_executable(runKeywordContainerTests KeywordContainerTests.cpp)
target_link_libraries(runKeywordContainerTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runKeywordContainerTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runKeywordContainerTests  )

add_executable(runDeckKWTests DeckKWTests.cpp)
target_link_libraries(runDeckKWTests Parser Logger ${Boost_LIBRARIES})
add_test(NAME runDeckKWTests  COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDeckKWTests  )

