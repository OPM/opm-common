opm_add_test(runOpmLogTests SOURCES OpmLogTests.cpp
                            EXE_NAME runOpmLogTests
                            LIBRARIES Parser ${Boost_LIBRARIES})
