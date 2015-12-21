foreach(tapp TableManagerTests TabdimsTests TableContainerTests
             ColumnSchemaTests)

  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES opmparser ${Boost_LIBRARIES})

endforeach()
