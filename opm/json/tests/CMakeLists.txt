opm_add_test(runjsonTests SOURCES jsonTests.cpp
                          LIBRARIES opm-json ${Boost_LIBRARIES})
set_source_files_properties( jsonTests.cpp PROPERTIES COMPILE_FLAGS "-Wno-unused-variable")
