add_definitions( -DKEYWORD_DIRECTORY="${PROJECT_SOURCE_DIR}/opm/parser/share/keywords")

add_executable(runIntegrationTests IntegrationTests.cpp)
target_link_libraries(runIntegrationTests Parser ${Boost_LIBRARIES})
add_test(NAME runIntegrationTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runIntegrationTests)
set_property(SOURCE IntegrationTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")


add_executable(runParseWCONHIST ParseWCONHIST.cpp)
target_link_libraries(runParseWCONHIST Parser ${Boost_LIBRARIES})
add_test(NAME runParseWCONHIST WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runParseWCONHIST)

add_executable(runParsePORO ParsePORO.cpp)
target_link_libraries(runParsePORO Parser ${Boost_LIBRARIES})
add_test(NAME runParsePORO WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runParsePORO) 

add_executable(runScheduleCreateFromDeck ScheduleCreateFromDeck.cpp)
target_link_libraries(runScheduleCreateFromDeck Parser ${Boost_LIBRARIES})
add_test(NAME runScheduleCreateFromDeck WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runScheduleCreateFromDeck) 

add_executable(runCompletionsFromDeck CompletionsFromDeck.cpp)
target_link_libraries(runCompletionsFromDeck Parser ${Boost_LIBRARIES})
add_test(NAME runCompletionsFromDeck WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runCompletionsFromDeck) 

add_executable(runParseACTION ParseACTION.cpp)
target_link_libraries(runParseACTION Parser ${Boost_LIBRARIES})
add_test(NAME runParseACTION WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runParseACTION)

add_executable(runIncludeTest IncludeTest.cpp)
target_link_libraries(runIncludeTest Parser ${Boost_LIBRARIES})
add_test(NAME runIncludeTest WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runIncludeTest)




add_executable(runParseEQUIL ParseEQUIL.cpp)
target_link_libraries(runParseEQUIL Parser ${Boost_LIBRARIES})
add_test(NAME runParseEQUIL 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParseEQUIL)

add_executable(runParseRSVD ParseRSVD.cpp)
target_link_libraries(runParseRSVD Parser ${Boost_LIBRARIES})
add_test(NAME runParseRSVD 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParseRSVD)
         
add_executable(runParsePVTG ParsePVTG.cpp)
target_link_libraries(runParsePVTG Parser ${Boost_LIBRARIES})
add_test(NAME runParsePVTG 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParsePVTG)         

if (INCLUDE_STATOIL_TESTS) 
   add_executable(runIntegrationTestsInternalData IntegrationTestsInternalData.cpp)
   target_link_libraries(runIntegrationTestsInternalData Parser ${Boost_LIBRARIES})
   add_test(NAME runIntegrationTestsInternalData WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runIntegrationTestsInternalData )
   set_property(TEST runIntegrationTestsInternalData PROPERTY LABELS Statoil)
endif()
