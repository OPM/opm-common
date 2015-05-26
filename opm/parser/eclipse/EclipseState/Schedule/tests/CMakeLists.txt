foreach(tapp TimeMapTest ScheduleTests WellTests
             WellPropertiesTests WellSetTests
             GroupTests ScheduleEnumTests
             CompletionTests CompletionSetTests
             DynamicStateTests GroupTreeNodeTests
             GroupTreeTests TuningTests)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES Parser ${Boost_LIBRARIES})
endforeach()
