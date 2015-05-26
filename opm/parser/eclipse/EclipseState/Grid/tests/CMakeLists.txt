foreach(tapp EclipseGridTests MULTREGTScannerTests GridPropertyTests
             FaceDirTests GridPropertiesTests BoxTests PORVTests
             BoxManagerTests TransMultTests FaultTests
             EqualRegTests MultiRegTests ADDREGTests CopyRegTests)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES Parser ${Boost_LIBRARIES})
endforeach()
