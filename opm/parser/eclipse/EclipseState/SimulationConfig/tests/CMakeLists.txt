foreach(tapp ThresholdPressureTest SimulationConfigTest)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES Parser ${Boost_LIBRARIES})
endforeach()
