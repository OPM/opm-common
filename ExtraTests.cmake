if(NOT TARGET Boost::unit_test_framework OR NOT BUILD_TESTING)
  return()
endif()

 # Generated source, needs to be here
opm_add_executable(
  TARGET
   TestKeywords
  SOURCES
    ${PROJECT_BINARY_DIR}/TestKeywords.cpp
  LIBRARIES
    opmcommon
    Boost::unit_test_framework
)
opm_add_test(InlineKeywordTest
  NO_COMPILE
  EXE_NAME
    TestKeywords
)

# Extra compile definitions and extra parameters
include(cmake/Modules/CheckCaseSensitiveFileSystem.cmake)
set(_testdir ${PROJECT_SOURCE_DIR}/tests/parser/data)

opm_add_executable(
  TARGET
    ParserTests
  SOURCES
    tests/parser/ParserTests.cpp
  LIBRARIES
    opmcommon
    Boost::unit_test_framework
)

opm_add_test(ParserTests
  NO_COMPILE
  TEST_ARGS
    -- ${_testdir}/
)

opm_add_executable(
  TARGET
    ParserIncludeTests
  SOURCES
    tests/parser/ParserIncludeTests.cpp
  LIBRARIES
    opmcommon
    Boost::unit_test_framework
)
target_compile_definitions(ParserIncludeTests
  PRIVATE
    -DHAVE_CASE_SENSITIVE_FILESYSTEM=${HAVE_CASE_SENSITIVE_FILESYSTEM}
)

opm_add_test(ParserIncludeTests
  NO_COMPILE
  TEST_ARGS
    -- ${_testdir}/parser/
)

 opm_add_executable(
   TARGET
     PvtxTableTests
   SOURCES
     tests/parser/PvtxTableTests.cpp
   LIBRARIES
      opmcommon
      Boost::unit_test_framework
)
opm_add_test(PvtxTableTests
  NO_COMPILE
  TEST_ARGS
    -- ${_testdir}/integration_tests/
)

opm_add_executable(
  TARGET
    EclipseStateTests
  SOURCES
    tests/parser/EclipseStateTests.cpp
  LIBRARIES
    opmcommon
    Boost::unit_test_framework
)
opm_add_test(EclipseStateTests
  NO_COMPILE
  TEST_ARGS
    -- ${_testdir}/integration_tests/
)

foreach(test
          BoxTest
          CarfinTest
          CheckDeckValidity
          EclipseGridCreateFromDeck
          IncludeTest
          IntegrationTests
          IOConfigIntegrationTest
          ParseKEYWORD
          Polymer
          ScheduleCreateFromDeck
          TransMultIntegrationTests
        )
  opm_add_executable(
    TARGET
      ${test}
    SOURCES
      tests/parser/integration/${test}.cpp
    LIBRARIES
      opmcommon
      Boost::unit_test_framework
  )
  opm_add_test(${test}
    NO_COMPILE
    TEST_ARGS
      -- ${_testdir}/integration_tests/
  )
endforeach()

add_test(
  NAME
    rst_deck_test
  COMMAND
    ${PROJECT_SOURCE_DIR}/tests/rst_test_driver.sh
    ${PROJECT_BINARY_DIR}/bin/rst_deck
    ${PROJECT_BINARY_DIR}/bin/opmhash
    ${PROJECT_SOURCE_DIR}/tests/SPE1CASE2_INCLUDE.DATA
)

add_test(
  NAME
    rst_deck_test2
  COMMAND
    ${PROJECT_SOURCE_DIR}/tests/rst_test_driver2.sh
    ${PROJECT_BINARY_DIR}/bin/rst_deck
    ${PROJECT_BINARY_DIR}/bin/opmi
    ${PROJECT_SOURCE_DIR}/tests/ACTIONX_M1.DATA
    ${PROJECT_SOURCE_DIR}/tests/ACTIONX_M1_MULTIPLE.DATA
    ${PROJECT_SOURCE_DIR}/tests/ACTIONX_M1.UNRST
    ${PROJECT_SOURCE_DIR}/tests/ACTIONX_M1.X0010
)

  # opm-tests dependent tests
if(HAVE_OPM_TESTS)
  opm_add_executable(
    TARGET
      parse_write
    SOURCES
      tests/parser/integration/parse_write.cpp
    LIBRARIES
      opmcommon
      Boost::unit_test_framework
  )
  foreach(deck
           ${OPM_TESTS_ROOT}/norne/NORNE_ATW2013.DATA
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
           ${OPM_TESTS_ROOT}/spe9/SPE9_CP_SHORT.DATA
           ${OPM_TESTS_ROOT}/spe9/SPE9.DATA
           ${OPM_TESTS_ROOT}/msw_2d_h/2D_H__.DATA
           ${OPM_TESTS_ROOT}/model2/0_BASE_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/1_MULTREGT_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/2_MULTXYZ_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/3_MULTFLT_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/4_MINPVV_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/5_SWATINIT_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/6_ENDSCALE_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/7_HYSTERESIS_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/8_MULTIPLY_TRANXYZ_MODEL2.DATA
           ${OPM_TESTS_ROOT}/model2/9_EDITNNC_MODEL2.DATA
         )
      get_filename_component(test_name ${deck} NAME_WE)
      opm_add_test(${test_name}
        NO_COMPILE
        EXE_NAME
          parse_write
        TEST_ARGS
          ${deck}
      )
  endforeach()
  opm_add_test("SPE9_CP_GROUP"
    NO_COMPILE
    EXE_NAME
      parse_write
    TEST_ARGS
      "${OPM_TESTS_ROOT}/spe9group/SPE9_CP_GROUP.DATA"
  )
  set_property(
    TEST
      NORNE_ATW2013
    PROPERTY
      ENVIRONMENT "OPM_ERRORS_IGNORE=PARSE_RANDOM_SLASH"
  )

  add_test(
    NAME
      rst_deck_test_norne
    COMMAND
      ${PROJECT_SOURCE_DIR}/tests/rst_test_driver.sh
      ${PROJECT_BINARY_DIR}/bin/rst_deck
      ${PROJECT_BINARY_DIR}/bin/opmhash
      ${OPM_TESTS_ROOT}/norne/NORNE_ATW2013.DATA
  )

  set_property(
    TEST
      rst_deck_test_norne
    PROPERTY
      ENVIRONMENT "OPM_ERRORS_IGNORE=PARSE_RANDOM_SLASH")
endif()

# JSON tests
opm_add_executable(
  TARGET
    jsonTests
  SOURCES
    tests/json/jsonTests.cpp
  LIBRARIES
    opmcommon
    Boost::unit_test_framework
)
opm_add_test(jsonTests
  NO_COMPILE
  TEST_ARGS
    ${PROJECT_SOURCE_DIR}/tests/json/example1.json
)
