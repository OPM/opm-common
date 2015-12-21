foreach(tapp TableManagerTests TabdimsTests TableContainerTests
             ColumnSchemaTests
             TableSchemaTests)

  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES opmparser ${Boost_LIBRARIES})

endforeach()
