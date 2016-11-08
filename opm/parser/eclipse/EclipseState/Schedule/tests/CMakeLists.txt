foreach(tapp TimeMapTest ScheduleTests WellTests WellPropertiesTests GroupTests
             ScheduleEnumTests CompletionTests CompletionSetTests
             DynamicStateTests GroupTreeTests TuningTests EventTests
             WellSolventTests DynamicVectorTests GeomodifierTests
             MessageLimitTests)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES opmparser ${Boost_LIBRARIES})
endforeach()
