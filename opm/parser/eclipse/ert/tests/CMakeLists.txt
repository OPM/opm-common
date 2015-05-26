opm_add_test(runErtTests SOURCES test_ert_wrapper.cpp
                         LIBRARIES Parser ${Boost_LIBRARIES})
