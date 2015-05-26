add_definitions( -DKEYWORD_DIRECTORY="${PROJECT_SOURCE_DIR}/opm/parser/share/keywords/000_Eclipse100")

foreach(tapp CheckDeckValidity IntegrationTests ParseWellProbe
             ParseTITLE ParseTOPS ParseWCONHIST ParseWellWithWildcards
             ParsePORO ParsePLYVISC ParsePLYSHLOG ParseDATAWithDefault
             ParseTVDP ParseDENSITY ParseVFPPROD ScheduleCreateFromDeck
             CompletionsFromDeck ParseEND IncludeTest ParseEQUIL
             ParseRSVD ParsePVTG ParsePVTO ParseSWOF BoxTest
             ParseMULTREGT ParseSGOF EclipseGridCreateFromDeck)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES opmparser ${Boost_LIBRARIES})
endforeach()
