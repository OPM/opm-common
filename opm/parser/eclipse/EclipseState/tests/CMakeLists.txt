foreach(tapp EclipseStateTests Eclipse3DPropertiesTests
             RunspecTests)

    opm_add_test(run${tapp}  SOURCES ${tapp}.cpp
                             LIBRARIES opmparser ${Boost_LIBRARIES})

endforeach()
