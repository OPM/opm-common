add_executable(runDeckRecordTests DeckRecordTests.cpp)
target_link_libraries(runDeckRecordTests Parser  ${Boost_LIBRARIES})
add_test(NAME runDeckRecordTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDeckRecordTests  )

add_executable(runDeckIntItemTests DeckIntItemTests.cpp)
target_link_libraries(runDeckIntItemTests Parser  ${Boost_LIBRARIES})
add_test(NAME runDeckIntItemTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDeckIntItemTests  )

add_executable(runDeckDoubleItemTests DeckDoubleItemTests.cpp)
target_link_libraries(runDeckDoubleItemTests Parser  ${Boost_LIBRARIES})
add_test(NAME runDeckDoubleItemTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDeckDoubleItemTests  )

add_executable(runDeckFloatItemTests DeckFloatItemTests.cpp)
target_link_libraries(runDeckFloatItemTests Parser  ${Boost_LIBRARIES})
add_test(NAME runDeckFloatItemTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDeckFloatItemTests  )

add_executable(runDeckStringItemTests DeckStringItemTests.cpp)
target_link_libraries(runDeckStringItemTests Parser  ${Boost_LIBRARIES})
add_test(NAME runDeckStringItemTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDeckStringItemTests  )

add_executable(runDeckTests DeckTests.cpp)
target_link_libraries(runDeckTests Parser  ${Boost_LIBRARIES})
add_test(NAME runDeckTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDeckTests  )

add_executable(runKeywordContainerTests KeywordContainerTests.cpp)
target_link_libraries(runKeywordContainerTests Parser  ${Boost_LIBRARIES})
add_test(NAME runKeywordContainerTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runKeywordContainerTests  )

add_executable(runDeckKeywordTests DeckKeywordTests.cpp)
target_link_libraries(runDeckKeywordTests Parser  ${Boost_LIBRARIES})
add_test(NAME runDeckKeywordTests  COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runDeckKeywordTests  )

