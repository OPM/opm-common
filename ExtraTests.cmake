# Libs to link tests against
set(TEST_LIBS opmcommon ecl Boost::unit_test_framework)
set(EXTRA_TESTS)

# Generated source, needs to be here
opm_add_test(InlineKeywordTest
             EXE_NAME inlinekw
             SOURCES ${PROJECT_BINARY_DIR}/inlinekw.cpp
             LIBRARIES ${TEST_LIBS})
list(APPEND EXTRA_TESTS inlinekw)

# Extra compile definitions and extra parameters
include(cmake/Modules/CheckCaseSensitiveFileSystem.cmake)
set(_testdir ${PROJECT_SOURCE_DIR}/tests/parser/data)

opm_add_test(LoaderTest
             SOURCES tests/parser/KeywordLoaderTests.cpp
                     src/opm/parser/eclipse/Generator/KeywordLoader.cpp
             LIBRARIES ${TEST_LIBS}
             TEST_ARGS ${_testdir}/parser/keyword-generator/)
list(APPEND EXTRA_TESTS LoaderTest)

opm_add_test(ParserTests
             SOURCES tests/parser/ParserTests.cpp
             LIBRARIES ${TEST_LIBS}
             TEST_ARGS ${_testdir}/)
list(APPEND EXTRA_TESTS ParserTests)

opm_add_test(ParserIncludeTests
             SOURCES tests/parser/ParserIncludeTests.cpp
             LIBRARIES ${TEST_LIBS}
             TEST_ARGS ${_testdir}/parser/)
target_compile_definitions(ParserIncludeTests PRIVATE
                           -DHAVE_CASE_SENSITIVE_FILESYSTEM=${HAVE_CASE_SENSITIVE_FILESYSTEM})
list(APPEND EXTRA_TESTS ParserIncludeTests)

opm_add_test(PvtxTableTests
             SOURCES tests/parser/PvtxTableTests.cpp
             LIBRARIES ${TEST_LIBS}
             TEST_ARGS ${_testdir}/integration_tests/)
list(APPEND EXTRA_TESTS PvtxTableTests)

opm_add_test(EclipseStateTests
             SOURCES tests/parser/EclipseStateTests.cpp
             LIBRARIES ${TEST_LIBS}
             TEST_ARGS ${_testdir}/integration_tests/)
list(APPEND EXTRA_TESTS EclipseStateTests)

foreach (test BoxTest
              CheckDeckValidity
              CompletionsFromDeck
              EclipseGridCreateFromDeck
              IncludeTest
              IntegrationTests
              IOConfigIntegrationTest
              NNCTests
              ParseKEYWORD
              ParseDATAWithDefault
              Polymer
              ResinsightTest
              ScheduleCreateFromDeck
              TransMultIntegrationTests)

  opm_add_test(${test}
               SOURCES tests/parser/integration/${test}.cpp
               LIBRARIES ${TEST_LIBS}
               TEST_ARGS ${_testdir}/integration_tests/)
  list(APPEND EXTRA_TESTS ${test})
endforeach ()

# opm-tests dependent tests
if(HAVE_OPM_TESTS)
  opm_add_test(parse_write ONLY_COMPILE
               SOURCES tests/parser/integration/parse_write.cpp
               LIBRARIES ${TEST_LIBS})
  list(APPEND EXTRA_TESTS parse_write)
  foreach (deck ${OPM_TESTS_ROOT}/norne/NORNE_ATW2013.DATA
                ${OPM_TESTS_ROOT}/spe1_solvent/SPE1CASE2_SOLVENT.DATA
                ${OPM_TESTS_ROOT}/spe9_solvent/SPE9_CP_SOLVENT_CO2.DATA
                ${OPM_TESTS_ROOT}/spe5/SPE5CASE1.DATA
                ${OPM_TESTS_ROOT}/polymer_simple2D/2D_THREEPHASE_POLY_HETER.DATA
                ${OPM_TESTS_ROOT}/spe1/SPE1CASE1.DATA
                ${OPM_TESTS_ROOT}/spe1/SPE1CASE2.DATA
                ${OPM_TESTS_ROOT}/spe1/SPE1CASE2_FAMII.DATA
                ${OPM_TESTS_ROOT}/spe1/SPE1CASE2_SLGOF.DATA
                ${OPM_TESTS_ROOT}/spe3/SPE3CASE1.DATA
                ${OPM_TESTS_ROOT}/spe3/SPE3CASE2.DATA
                ${OPM_TESTS_ROOT}/spe9/SPE9_CP.DATA
                ${OPM_TESTS_ROOT}/spe9/SPE9_CP_GROUP.DATA
                ${OPM_TESTS_ROOT}/spe9/SPE9.DATA
                ${OPM_TESTS_ROOT}/spe10model1/SPE10_MODEL1.DATA
                ${OPM_TESTS_ROOT}/spe10model2/SPE10_MODEL2.DATA
                ${OPM_TESTS_ROOT}/msw_2d_h/2D_H__.DATA )
      get_filename_component(test_name ${deck} NAME_WE)
      opm_add_test(${test_name} NO_COMPILE
                   EXE_NAME parse_write
                   TEST_ARGS ${deck})
  endforeach()
  set_property(TEST NORNE_ATW2013
               PROPERTY ENVIRONMENT "OPM_ERRORS_IGNORE=PARSE_RANDOM_SLASH")
endif()

# JSON tests
opm_add_test(jsonTests
             SOURCES tests/json/jsonTests.cpp
             LIBRARIES ${TEST_LIBS}
             TEST_ARGS ${PROJECT_SOURCE_DIR}/tests/json/example1.json)
list(APPEND EXTRA_TESTS jsonTests)
