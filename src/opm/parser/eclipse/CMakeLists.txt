project(opm-parser-eclipse CXX)

add_executable(genkw Parser/createDefaultKeywordList.cpp
                     Deck/Deck.cpp
                     Deck/DeckItem.cpp
                     Deck/DeckKeyword.cpp
                     Deck/DeckRecord.cpp
                     Generator/KeywordGenerator.cpp
                     Generator/KeywordLoader.cpp
                     Parser/MessageContainer.cpp
                     Parser/ParseContext.cpp
                     Parser/ParserEnums.cpp
                     Parser/ParserItem.cpp
                     Parser/ParserKeyword.cpp
                     Parser/ParserRecord.cpp
                     RawDeck/RawKeyword.cpp
                     RawDeck/RawRecord.cpp
                     RawDeck/StarToken.cpp
                     Units/Dimension.cpp
                     Units/UnitSystem.cpp
                     Utility/Stringview.cpp
)

target_link_libraries(genkw opmjson ecl boost_regex)
target_include_directories(genkw PRIVATE include)

file(GLOB_RECURSE keyword_templates share/keywords/*)
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords0.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords1.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords2.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords3.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/inlinekw.cpp
    COMMAND genkw ${PROJECT_SOURCE_DIR}/share/keywords
                  ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords.cpp
                  ${CMAKE_CURRENT_BINARY_DIR}/include/
                  opm/parser/eclipse/Parser/ParserKeywords
                  ${CMAKE_CURRENT_BINARY_DIR}/inlinekw.cpp
                  4
    DEPENDS genkw
)

#-----------------------------------------------------------------

add_library(opmparser ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords.cpp
                      ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords0.cpp
                      ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords1.cpp
                      ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords2.cpp
                      ${CMAKE_CURRENT_BINARY_DIR}/ParserKeywords3.cpp
                      Deck/Deck.cpp
                      Deck/DeckItem.cpp
                      Deck/DeckKeyword.cpp
                      Deck/DeckRecord.cpp
                      Deck/Section.cpp
                      EclipseState/checkDeck.cpp
                      EclipseState/Eclipse3DProperties.cpp
                      EclipseState/EclipseConfig.cpp
                      EclipseState/EclipseState.cpp
                      EclipseState/EndpointScaling.cpp
                      EclipseState/Grid/Box.cpp
                      EclipseState/Grid/BoxManager.cpp
                      EclipseState/Grid/EclipseGrid.cpp
                      EclipseState/Grid/FaceDir.cpp
                      EclipseState/Grid/FaultCollection.cpp
                      EclipseState/Grid/Fault.cpp
                      EclipseState/Grid/FaultFace.cpp
                      EclipseState/Grid/GridDims.cpp
                      EclipseState/Grid/GridProperties.cpp
                      EclipseState/Grid/GridProperty.cpp
                      EclipseState/Grid/MULTREGTScanner.cpp
                      EclipseState/Grid/NNC.cpp
                      EclipseState/Grid/PinchMode.cpp
                      EclipseState/Grid/SatfuncPropertyInitializers.cpp
                      EclipseState/Grid/TransMult.cpp
                      EclipseState/InitConfig/Equil.cpp
                      EclipseState/InitConfig/InitConfig.cpp
                      EclipseState/IOConfig/IOConfig.cpp
                      EclipseState/IOConfig/RestartConfig.cpp
                      EclipseState/Runspec.cpp
                      EclipseState/Schedule/Completion.cpp
                      EclipseState/Schedule/CompletionSet.cpp
                      EclipseState/Schedule/Events.cpp
                      EclipseState/Schedule/Group.cpp
                      EclipseState/Schedule/GroupTree.cpp
                      EclipseState/Schedule/MessageLimits.cpp
                      EclipseState/Schedule/MSW/Compsegs.cpp
                      EclipseState/Schedule/MSW/Segment.cpp
                      EclipseState/Schedule/MSW/SegmentSet.cpp
                      EclipseState/Schedule/OilVaporizationProperties.cpp
                      EclipseState/Schedule/Schedule.cpp
                      EclipseState/Schedule/ScheduleEnums.cpp
                      EclipseState/Schedule/TimeMap.cpp
                      EclipseState/Schedule/Tuning.cpp
                      EclipseState/Schedule/Well.cpp
                      EclipseState/Schedule/WellEconProductionLimits.cpp
                      EclipseState/Schedule/WellInjectionProperties.cpp
                      EclipseState/Schedule/WellPolymerProperties.cpp
                      EclipseState/Schedule/WellProductionProperties.cpp
                      EclipseState/SimulationConfig/SimulationConfig.cpp
                      EclipseState/SimulationConfig/ThresholdPressure.cpp
                      EclipseState/SummaryConfig/SummaryConfig.cpp
                      EclipseState/Tables/ColumnSchema.cpp
                      EclipseState/Tables/JFunc.cpp
                      EclipseState/Tables/PvtxTable.cpp
                      EclipseState/Tables/SimpleTable.cpp
                      EclipseState/Tables/TableColumn.cpp
                      EclipseState/Tables/TableContainer.cpp
                      EclipseState/Tables/TableIndex.cpp
                      EclipseState/Tables/TableManager.cpp
                      EclipseState/Tables/TableSchema.cpp
                      EclipseState/Tables/Tables.cpp
                      EclipseState/Tables/VFPInjTable.cpp
                      EclipseState/Tables/VFPProdTable.cpp
                      Parser/MessageContainer.cpp
                      Parser/ParseContext.cpp
                      Parser/Parser.cpp
                      Parser/ParserEnums.cpp
                      Parser/ParserItem.cpp
                      Parser/ParserKeyword.cpp
                      Parser/ParserRecord.cpp
                      RawDeck/RawKeyword.cpp
                      RawDeck/RawRecord.cpp
                      RawDeck/StarToken.cpp
                      Units/Dimension.cpp
                      Units/UnitSystem.cpp
                      Utility/Functional.cpp
                      Utility/Stringview.cpp
)

target_link_libraries(opmparser PUBLIC opmjson
                                       ecl
                                       boost_filesystem
                                       boost_system
                                       boost_regex
                                       boost_date_time)
target_compile_definitions(opmparser PRIVATE -DOPM_PARSER_DECK_API=1)
target_include_directories(opmparser
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include
)

install(TARGETS opmparser
        EXPORT  opm-parser-config
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/
        DESTINATION include
        PATTERN *.hpp
)

set_target_properties(opmparser PROPERTIES
    VERSION   ${opm-parser_VERSION_MAJOR}.${opm-parser_VERSION_MINOR}
    SOVERSION ${opm-parser_VERSION_MAJOR}
)

if (NOT BUILD_TESTING)
    return ()
endif ()

set(_testdir ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)

add_executable(inlinekw ${CMAKE_CURRENT_BINARY_DIR}/inlinekw.cpp)
target_link_libraries(inlinekw opmparser boost_test)
add_test(NAME InlineKeywordTest COMMAND inlinekw)

add_executable(LoaderTest tests/KeywordLoaderTests.cpp Generator/KeywordLoader.cpp)
target_link_libraries(LoaderTest opmparser boost_test)
add_test(NAME LoaderTest
         COMMAND LoaderTest ${_testdir}/parser/keyword-generator/)

add_executable(ParserTests tests/ParserTests.cpp)
target_link_libraries(ParserTests opmparser boost_test)
add_test(NAME ParserTests COMMAND ParserTests ${_testdir}/)

add_executable(ParserIncludeTests tests/ParserIncludeTests.cpp)
target_compile_definitions(ParserIncludeTests PRIVATE
    -DHAVE_CASE_SENSITIVE_FILESYSTEM=${HAVE_CASE_SENSITIVE_FILESYSTEM}
)
target_link_libraries(ParserIncludeTests opmparser boost_test)
add_test(NAME ParserIncludeTests COMMAND ParserIncludeTests ${_testdir}/parser/)

add_executable(PvtxTableTests tests/PvtxTableTests.cpp)
target_link_libraries(PvtxTableTests opmparser boost_test)
add_test(NAME PvtxTableTests
         COMMAND PvtxTableTests ${_testdir}/integration_tests/)

add_executable(EclipseStateTests tests/EclipseStateTests.cpp)
target_link_libraries(EclipseStateTests opmparser boost_test)
add_test(NAME EclipseStateTests
         COMMAND EclipseStateTests ${_testdir}/integration_tests/)

foreach(test ADDREGTests
             BoxTests
             ColumnSchemaTests
             CompletionTests
             COMPSEGUnits
             CopyRegTests
             DeckTests
             DynamicStateTests
             DynamicVectorTests
             Eclipse3DPropertiesTests
             EclipseGridTests
             EqualRegTests
             EventTests
             FaceDirTests
             FaultTests
             FunctionalTests
             GeomodifierTests
             GridPropertyTests
             GroupTests
             InitConfigTest
             IOConfigTests
             MessageContainerTest
             MessageLimitTests
             MultiRegTests
             MULTREGTScannerTests
             OrderedMapTests
             ParseContextTests
             PORVTests
             RawKeywordTests
             RestartConfigTests
             RunspecTests
             SatfuncPropertyInitializersTests
             ScheduleTests
             SectionTests
             SimpleTableTests
             SimulationConfigTest
             StarTokenTests
             StringTests
             SummaryConfigTests
             TabdimsTests
             TableColumnTests
             TableContainerTests
             TableManagerTests
             TableSchemaTests
             ThresholdPressureTest
             TimeMapTest
             TransMultTests
             TuningTests
             UnitTests
             ValueTests
             WellSolventTests
             WellTests
)
    add_executable(${test} tests/${test}.cpp)
    target_link_libraries(${test} opmparser boost_test)
    add_test(NAME ${test} COMMAND ${test})
endforeach ()

foreach (test BoxTest
              CheckDeckValidity
              CompletionsFromDeck
              EclipseGridCreateFromDeck
              IncludeTest
              IntegrationTests
              IOConfigIntegrationTest
              NNCTests
              ParseKEYWORD
              ParseDATAWithDefault
              Polymer
              ResinsightTest
              ScheduleCreateFromDeck
              TransMultIntegrationTests
)
    add_executable(${test} IntegrationTests/${test}.cpp)
    target_link_libraries(${test} opmparser boost_test)
    add_test(NAME ${test} COMMAND ${test} ${_testdir}/integration_tests/)
endforeach ()

if (NOT HAVE_OPM_DATA)
    return ()
endif ()

foreach (deck ${OPM_DATA_ROOT}/norne/NORNE_ATW2013.DATA
              ${OPM_DATA_ROOT}/solvent_test_suite/SPE1CASE2_SOLVENT.DATA
              ${OPM_DATA_ROOT}/solvent_test_suite/SPE9_CP_SOLVENT_CO2.DATA
              ${OPM_DATA_ROOT}/solvent_test_suite/SPE5CASE1.DATA
              ${OPM_DATA_ROOT}/polymer_test_suite/simple2D/2D_THREEPHASE_POLY_HETER.DATA
              ${OPM_DATA_ROOT}/spe1/SPE1CASE1.DATA
              ${OPM_DATA_ROOT}/spe1/SPE1CASE2.DATA
              ${OPM_DATA_ROOT}/spe1/SPE1CASE2_FAMII.DATA
              ${OPM_DATA_ROOT}/spe1/SPE1CASE2_SLGOF.DATA
              ${OPM_DATA_ROOT}/spe3/SPE3CASE1.DATA
              ${OPM_DATA_ROOT}/spe3/SPE3CASE2.DATA
              ${OPM_DATA_ROOT}/spe9/SPE9_CP.DATA
              ${OPM_DATA_ROOT}/spe9/SPE9_CP_GROUP.DATA
              ${OPM_DATA_ROOT}/spe9/SPE9.DATA
              ${OPM_DATA_ROOT}/spe10model1/SPE10_MODEL1.DATA
              ${OPM_DATA_ROOT}/spe10model2/SPE10_MODEL2.DATA
              ${OPM_DATA_ROOT}/msw_2d_h/2D_H__.DATA )
    get_filename_component(test_name ${deck} NAME_WE)
    add_test(NAME ${test_name} COMMAND opmi ${deck})
endforeach()
set_property(TEST NORNE_ATW2013
             PROPERTY ENVIRONMENT "OPM_ERRORS_IGNORE=PARSE_RANDOM_SLASH")
