foreach(tapp OrderedMapTests ValueTests ElasticVectorTests)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES Parser ${Boost_LIBRARIES})
endforeach()
