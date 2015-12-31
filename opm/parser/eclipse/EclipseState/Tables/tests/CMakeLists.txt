foreach(tapp TableManagerTests
             TabdimsTests
             TableContainerTests
             SimpleTableTests
             ColumnSchemaTests
             TableSchemaTests
             TableColumnTests)

  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES opmparser ${Boost_LIBRARIES})

endforeach()
