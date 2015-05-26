foreach(tapp DeckRecordTests DeckIntItemTests DeckDoubleItemTests
             DeckFloatItemTests DeckStringItemTests
             DeckTests DeckKeywordTests SectionTests)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES Parser ${Boost_LIBRARIES})
endforeach()
