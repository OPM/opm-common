include_directories(BEFORE ${PROJECT_BINARY_DIR}/generated-source/include)

add_subdirectory(OpmLog/tests)
add_subdirectory(Parser/tests)
add_subdirectory(Generator/tests)
add_subdirectory(RawDeck/tests)
add_subdirectory(Deck/tests)
add_subdirectory(Units/tests)
add_subdirectory(EclipseState/tests)
add_subdirectory(EclipseState/Schedule/tests)
add_subdirectory(EclipseState/SimulationConfig/tests)
add_subdirectory(EclipseState/Tables/tests)
add_subdirectory(EclipseState/Grid/tests)
add_subdirectory(EclipseState/Util/tests)
add_subdirectory(EclipseState/IOConfig/tests)
add_subdirectory(EclipseState/InitConfig/tests)

add_subdirectory(Applications)
add_subdirectory(IntegrationTests)

set( log_source
OpmLog/TimerLog.cpp 
OpmLog/CounterLog.cpp 
OpmLog/LogUtil.cpp
OpmLog/Logger.cpp 
OpmLog/LogBackend.cpp 
OpmLog/StreamLog.cpp 
OpmLog/OpmLog.cpp )

set( rawdeck_source 
RawDeck/StarToken.cpp
RawDeck/RawKeyword.cpp 
RawDeck/RawRecord.cpp )

set( unit_source
Units/UnitSystem.cpp
Units/Dimension.cpp)

set( deck_source
Deck/Deck.cpp
Deck/DeckKeyword.cpp
Deck/DeckRecord.cpp
Deck/DeckItem.cpp
Deck/DeckIntItem.cpp
Deck/DeckDoubleItem.cpp
Deck/DeckFloatItem.cpp
Deck/DeckStringItem.cpp
Deck/Section.cpp
Deck/SCHEDULESection.cpp
Deck/DeckTimeStep.cpp
)

set( parser_source
Parser/ParseMode.cpp
Parser/ParserEnums.cpp
Parser/ParserKeyword.cpp 
Parser/Parser.cpp 
Parser/ParserRecord.cpp
Parser/ParserItem.cpp
Parser/ParserIntItem.cpp  
Parser/ParserFloatItem.cpp
Parser/ParserDoubleItem.cpp
Parser/ParserStringItem.cpp
)

set( generator_source
Generator/KeywordGenerator.cpp
Generator/KeywordLoader.cpp )

set( build_parser_source 
Parser/ParseMode.cpp
Parser/ParserEnums.cpp
Parser/ParserKeyword.cpp 
Parser/ParserRecord.cpp
Parser/ParserItem.cpp
Parser/ParserIntItem.cpp  
Parser/ParserFloatItem.cpp
Parser/ParserDoubleItem.cpp
Parser/ParserStringItem.cpp  
${generator_source}
${log_source}
)

set (state_source
EclipseState/EclipseState.cpp
#
EclipseState/checkDeck.cpp
#
EclipseState/Schedule/OilVaporizationProperties.cpp
EclipseState/Schedule/TimeMap.cpp 
EclipseState/Schedule/Schedule.cpp 
EclipseState/Schedule/Well.cpp
EclipseState/Schedule/WellProductionProperties.cpp
EclipseState/Schedule/WellInjectionProperties.cpp
EclipseState/Schedule/WellPolymerProperties.cpp
EclipseState/Schedule/MSW/Segment.cpp
EclipseState/Schedule/MSW/SegmentSet.cpp
EclipseState/Schedule/MSW/Compsegs.cpp
EclipseState/Schedule/WellSet.cpp
EclipseState/Schedule/Group.cpp
EclipseState/Schedule/Completion.cpp
EclipseState/Schedule/CompletionSet.cpp
EclipseState/Schedule/ScheduleEnums.cpp 
EclipseState/Schedule/GroupTreeNode.cpp
EclipseState/Schedule/GroupTree.cpp
EclipseState/Schedule/Tuning.cpp
EclipseState/Schedule/Events.cpp
#
EclipseState/Tables/SimpleTable.cpp
EclipseState/Tables/VFPProdTable.cpp
EclipseState/Tables/VFPInjTable.cpp
EclipseState/Tables/TableManager.cpp
EclipseState/Tables/TableContainer.cpp
EclipseState/Tables/TableColumn.cpp
EclipseState/Tables/ColumnSchema.cpp
EclipseState/Tables/TableSchema.cpp
EclipseState/Tables/TableIndex.cpp
EclipseState/Tables/PvtxTable.cpp
EclipseState/Tables/Tables.cpp
#
EclipseState/Grid/GridProperty.cpp
EclipseState/Grid/Box.cpp
EclipseState/Grid/BoxManager.cpp
EclipseState/Grid/FaceDir.cpp
EclipseState/Grid/TransMult.cpp        
EclipseState/Grid/MULTREGTScanner.cpp        
EclipseState/Grid/EclipseGrid.cpp
EclipseState/Grid/FaultFace.cpp
EclipseState/Grid/Fault.cpp
EclipseState/Grid/FaultCollection.cpp
EclipseState/Grid/NNC.cpp
EclipseState/Grid/PinchMode.cpp
#
EclipseState/InitConfig/InitConfig.cpp
EclipseState/SimulationConfig/SimulationConfig.cpp
EclipseState/SimulationConfig/ThresholdPressure.cpp
EclipseState/IOConfig/IOConfig.cpp)
#

set( HEADER_FILES
OpmLog/LogBackend.hpp
OpmLog/TimerLog.hpp 
OpmLog/CounterLog.hpp
OpmLog/Logger.hpp
OpmLog/OpmLog.hpp
OpmLog/LogUtil.hpp
OpmLog/StreamLog.hpp
#
RawDeck/RawConsts.hpp 
RawDeck/RawKeyword.hpp 
RawDeck/RawRecord.hpp 
RawDeck/StarToken.hpp
RawDeck/RawEnums.hpp
#
Deck/Deck.hpp
Deck/DeckKeyword.hpp
Deck/DeckRecord.hpp
Deck/DeckItem.hpp
Deck/DeckIntItem.hpp
Deck/DeckDoubleItem.hpp
Deck/DeckFloatItem.hpp
Deck/DeckStringItem.hpp
Deck/Section.hpp
Deck/SCHEDULESection.hpp
Deck/DeckTimeStep.hpp
#
Parser/ParserEnums.hpp
Parser/ParserKeyword.hpp 
Parser/Parser.hpp 
Parser/ParserRecord.hpp
Parser/ParserItem.hpp
Parser/ParserIntItem.hpp  
Parser/ParserFloatItem.hpp
Parser/ParserDoubleItem.hpp
Parser/ParserStringItem.hpp
Parser/InputErrorAction.hpp
Parser/ParseMode.hpp
#
Generator/KeywordLoader.hpp
Generator/KeywordGenerator.hpp
#
Units/UnitSystem.hpp
Units/Dimension.hpp
Units/ConversionFactors.hpp
#
EclipseState/EclipseState.hpp
#
EclipseState/checkDeck.hpp
#
EclipseState/Schedule/OilVaporizationProperties.hpp
EclipseState/Schedule/TimeMap.hpp
EclipseState/Schedule/Schedule.hpp
EclipseState/Schedule/Well.hpp
EclipseState/Schedule/WellProductionProperties.hpp
EclipseState/Schedule/WellInjectionProperties.hpp
EclipseState/Schedule/WellPolymerProperties.hpp
EclipseState/Schedule/MSW/Segment.hpp
EclipseState/Schedule/MSW/SegmentSet.hpp
EclipseState/Schedule/MSW/Compsegs.hpp
EclipseState/Schedule/WellSet.hpp
EclipseState/Schedule/Group.hpp
EclipseState/Schedule/DynamicState.hpp
EclipseState/Schedule/DynamicVector.hpp
EclipseState/Schedule/Completion.hpp
EclipseState/Schedule/CompletionSet.hpp
EclipseState/Schedule/ScheduleEnums.hpp
EclipseState/Schedule/GroupTreeNode.hpp 
EclipseState/Schedule/GroupTree.hpp
EclipseState/Schedule/Tuning.hpp
EclipseState/Schedule/Events.hpp
#
EclipseState/Util/RecordVector.hpp 
EclipseState/Util/OrderedMap.hpp 
EclipseState/Util/Value.hpp 
#
EclipseState/Grid/EclipseGrid.hpp
EclipseState/Grid/GridProperty.hpp
EclipseState/Grid/GridProperties.hpp
EclipseState/Grid/GridPropertyInitializers.hpp
EclipseState/Grid/SatfuncPropertyInitializers.hpp
EclipseState/Grid/Box.hpp
EclipseState/Grid/BoxManager.hpp
EclipseState/Grid/FaceDir.hpp
EclipseState/Grid/MinpvMode.hpp
EclipseState/Grid/PinchMode.hpp
EclipseState/Grid/MULTREGTScanner.hpp        
EclipseState/Grid/TransMult.hpp       
EclipseState/Grid/FaultFace.hpp
EclipseState/Grid/Fault.hpp
EclipseState/Grid/FaultCollection.hpp
EclipseState/Grid/NNC.hpp
#
EclipseState/InitConfig/InitConfig.hpp
EclipseState/SimulationConfig/SimulationConfig.hpp
EclipseState/SimulationConfig/ThresholdPressure.hpp
EclipseState/IOConfig/IOConfig.hpp
#
EclipseState/Tables/Tabdims.hpp
EclipseState/Tables/Eqldims.hpp
EclipseState/Tables/Regdims.hpp
EclipseState/Tables/PlyadsTable.hpp
EclipseState/Tables/PvtoTable.hpp
EclipseState/Tables/RocktabTable.hpp
EclipseState/Tables/PvdoTable.hpp
EclipseState/Tables/PvdgTable.hpp
EclipseState/Tables/PvdsTable.hpp
EclipseState/Tables/SimpleTable.hpp
EclipseState/Tables/PlymaxTable.hpp
EclipseState/Tables/PlyrockTable.hpp
EclipseState/Tables/SwofTable.hpp
EclipseState/Tables/SgwfnTable.hpp
EclipseState/Tables/SwfnTable.hpp
EclipseState/Tables/SgfnTable.hpp
EclipseState/Tables/SsfnTable.hpp
EclipseState/Tables/Sof2Table.hpp
EclipseState/Tables/Sof3Table.hpp
EclipseState/Tables/EnptvdTable.hpp
EclipseState/Tables/PlyviscTable.hpp
EclipseState/Tables/PlydhflfTable.hpp
EclipseState/Tables/PlyshlogTable.hpp
EclipseState/Tables/EnkrvdTable.hpp
EclipseState/Tables/ImkrvdTable.hpp
EclipseState/Tables/SgofTable.hpp
EclipseState/Tables/SlgofTable.hpp
EclipseState/Tables/PvtxTable.hpp
EclipseState/Tables/ImptvdTable.hpp
EclipseState/Tables/RsvdTable.hpp
EclipseState/Tables/RvvdTable.hpp
EclipseState/Tables/RtempvdTable.hpp
EclipseState/Tables/OilvisctTable.hpp
EclipseState/Tables/GasvisctTable.hpp
EclipseState/Tables/WatvisctTable.hpp
EclipseState/Tables/PvtgTable.hpp
EclipseState/Tables/VFPProdTable.hpp
EclipseState/Tables/VFPInjTable.hpp
EclipseState/Tables/TableManager.hpp
EclipseState/Tables/TableContainer.hpp
EclipseState/Tables/SorwmisTable.hpp
EclipseState/Tables/SgcwmisTable.hpp
EclipseState/Tables/MiscTable.hpp
EclipseState/Tables/PmiscTable.hpp
EclipseState/Tables/MsfnTable.hpp
EclipseState/Tables/TableColumn.hpp
EclipseState/Tables/ColumnSchema.hpp
EclipseState/Tables/TableEnums.hpp
EclipseState/Tables/TableSchema.hpp
EclipseState/Tables/TableIndex.hpp
#
Utility/WconinjeWrapper.hpp
Utility/CompdatWrapper.hpp
Utility/WconinjWrapper.hpp
Utility/WgrupconWrapper.hpp
Utility/GconprodWrapper.hpp
Utility/TstepWrapper.hpp
Utility/StartWrapper.hpp
Utility/SpecgridWrapper.hpp
Utility/WconprodWrapper.hpp
Utility/GconinjeWrapper.hpp
Utility/WelopenWrapper.hpp
Utility/GruptreeWrapper.hpp
Utility/WelspecsWrapper.hpp
Utility/EquilWrapper.hpp
Utility/EndscaleWrapper.hpp
Utility/ScalecrsWrapper.hpp)

add_library(buildParser ${rawdeck_source} ${build_parser_source} ${deck_source} ${unit_source} ${generator_source})
target_link_libraries(buildParser opmjson ${Boost_LIBRARIES}  ${ERT_LIBRARIES})

#-----------------------------------------------------------------
# This section manages the generation of C++ code for the default keywords.

# 1. Create an executable 'createDefaultKeywordList'.

add_executable( createDefaultKeywordList Parser/createDefaultKeywordList.cpp )
target_link_libraries( createDefaultKeywordList buildParser opmjson ${Boost_LIBRARIES})



# 2. Run the generated application createDefaultKeywordlist - this
#    application will recursively scan through all the json keyword
#    files in the source tree. It will maintain a signature of these json files,
#    and only update the generated files if the json files have changed.
#
#    This target will always run - the dependency "management" is
#    implicitly handled in the createDefaultKeywordList application.

set( generated_source ${PROJECT_BINARY_DIR}/generated-source/ParserKeywords.cpp)
# to tune the number of files addDefaultKeywords is split into, change
# generated_num_files. 4 is a very reasonable estimate.
# TODO: support from command line options?
set( generated_num_files 4 )
math( EXPR generated_num_files_end "${generated_num_files} - 1" )
foreach( n RANGE ${generated_num_files_end} )
    list(APPEND generated_source ${PROJECT_BINARY_DIR}/generated-source/ParserKeywords${n}.cpp)
    set_source_files_properties(${PROJECT_BINARY_DIR}/generated-source/ParserKeywords${n}.cpp PROPERTIES GENERATED TRUE)
endforeach( n )

set_source_files_properties(${PROJECT_BINARY_DIR}/generated-source/ParserKeywords.cpp PROPERTIES GENERATED TRUE)
set_source_files_properties(${PROJECT_BINARY_DIR}/generated-source/inlcude/opm/parser/eclipse/Parser/ParserKeywords.hpp PROPERTIES GENERATED TRUE)
set_source_files_properties(${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp PROPERTIES GENERATED TRUE)

add_custom_target( generatedCode ALL COMMAND createDefaultKeywordList
                   ${PROJECT_SOURCE_DIR}/opm/parser/share/keywords
                   ${PROJECT_BINARY_DIR}/generated-source/ParserKeywords.cpp
                   ${PROJECT_BINARY_DIR}/generated-source/include/
                   opm/parser/eclipse/Parser/ParserKeywords
                   ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp
                   ${generated_num_files}
                )

opm_add_test( runInlineKeywordTest SOURCES ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp
              LIBRARIES opmparser ${Boost_LIBRARIES}
              DEPENDS generatedCode )

#-----------------------------------------------------------------

add_library(opmparser ${generated_source} ${state_source} ${rawdeck_source} ${parser_source} ${deck_source} ${unit_source} ${log_source} ${generator_source})
add_dependencies( opmparser generatedCode )
target_link_libraries(opmparser opmjson ${Boost_LIBRARIES}  ${ERT_LIBRARIES})
set_target_properties(opmparser PROPERTIES VERSION ${opm-parser_VERSION_MAJOR}.${opm-parser_VERSION_MINOR}
                                           SOVERSION ${opm-parser_VERSION_MAJOR})

include( ${PROJECT_SOURCE_DIR}/cmake/Modules/install_headers.cmake )   
install_headers( "${HEADER_FILES}" "${CMAKE_INSTALL_PREFIX}" )
install( TARGETS opmparser DESTINATION ${CMAKE_INSTALL_LIBDIR} )


FILE( GLOB generated_header
        ${PROJECT_BINARY_DIR}/generated-source/include/opm/parser/eclipse/Parser/ParserKeywords.hpp
        ${PROJECT_BINARY_DIR}/generated-source/include/opm/parser/eclipse/Parser/ParserKeywords/*.hpp
    )
install(FILES ${generated_header} DESTINATION include/opm/parser/eclipse/Parser)
if (ENABLE_PYTHON)
   add_subdirectory( python )
endif()
