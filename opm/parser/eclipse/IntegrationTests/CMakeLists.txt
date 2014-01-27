add_definitions( -DKEYWORD_DIRECTORY="${PROJECT_SOURCE_DIR}/opm/parser/share/keywords")

add_executable(runIntegrationTests IntegrationTests.cpp)
target_link_libraries(runIntegrationTests Parser ${Boost_LIBRARIES})
add_test(NAME runIntegrationTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runIntegrationTests)
set_property(SOURCE IntegrationTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")


add_executable(runParseWCONHIST ParseWCONHIST.cpp)
target_link_libraries(runParseWCONHIST Parser ${Boost_LIBRARIES})
add_test(NAME runParseWCONHIST WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseWCONHIST)

add_executable(runParsePORO ParsePORO.cpp)
target_link_libraries(runParsePORO Parser ${Boost_LIBRARIES})
add_test(NAME runParsePORO WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParsePORO) 

add_executable(runParseTVDP ParseTVDP.cpp)
target_link_libraries(runParseTVDP Parser ${Boost_LIBRARIES})
add_test(NAME runParseTVDP WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseTVDP) 

add_executable(runParseDENSITY ParseDENSITY.cpp)
target_link_libraries(runParseDENSITY Parser ${Boost_LIBRARIES})
add_test(NAME runParseDENSITY WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseDENSITY) 

add_executable(runParseVFPPROD ParseVFPPROD.cpp)
target_link_libraries(runParseVFPPROD Parser ${Boost_LIBRARIES})
add_test(NAME runParseVFPPROD WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseVFPPROD) 

add_executable(runScheduleCreateFromDeck ScheduleCreateFromDeck.cpp)
target_link_libraries(runScheduleCreateFromDeck Parser ${Boost_LIBRARIES})
add_test(NAME runScheduleCreateFromDeck WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runScheduleCreateFromDeck) 

add_executable(runCompletionsFromDeck CompletionsFromDeck.cpp)
target_link_libraries(runCompletionsFromDeck Parser ${Boost_LIBRARIES})
add_test(NAME runCompletionsFromDeck WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runCompletionsFromDeck) 

add_executable(runParseACTION ParseACTION.cpp)
target_link_libraries(runParseACTION Parser ${Boost_LIBRARIES})
add_test(NAME runParseACTION WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseACTION)

add_executable(runIncludeTest IncludeTest.cpp)
target_link_libraries(runIncludeTest Parser ${Boost_LIBRARIES})
add_test(NAME runIncludeTest WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runIncludeTest)




add_executable(runParseEQUIL ParseEQUIL.cpp)
target_link_libraries(runParseEQUIL Parser ${Boost_LIBRARIES})
add_test(NAME runParseEQUIL 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseEQUIL)

add_executable(runParseRSVD ParseRSVD.cpp)
target_link_libraries(runParseRSVD Parser ${Boost_LIBRARIES})
add_test(NAME runParseRSVD 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseRSVD)
         
add_executable(runParsePVTG ParsePVTG.cpp)
target_link_libraries(runParsePVTG Parser ${Boost_LIBRARIES})
add_test(NAME runParsePVTG 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParsePVTG)         

add_executable(runParsePVTO ParsePVTO.cpp)
target_link_libraries(runParsePVTO Parser ${Boost_LIBRARIES})
add_test(NAME runParsePVTO 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParsePVTO)         

add_executable(runParseSWOF ParseSWOF.cpp)
target_link_libraries(runParseSWOF Parser ${Boost_LIBRARIES})
add_test(NAME runParseSWOF 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseSWOF)         

add_executable(runParseSGOF ParseSGOF.cpp)
target_link_libraries(runParseSGOF Parser ${Boost_LIBRARIES})
add_test(NAME runParseSGOF 
         WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} 
         COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParseSGOF)         

if (INCLUDE_STATOIL_TESTS) 
   add_executable(runIntegrationTestsInternalData IntegrationTestsInternalData.cpp)
   target_link_libraries(runIntegrationTestsInternalData Parser ${Boost_LIBRARIES})
   add_test(NAME runIntegrationTestsInternalData WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runIntegrationTestsInternalData )
   set_property(TEST runIntegrationTestsInternalData PROPERTY LABELS Statoil)
endif()
