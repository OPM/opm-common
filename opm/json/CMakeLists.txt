add_subdirectory( tests )

enable_language( C )
set( json_source JsonObject.cpp cjson/cJSON.c )

add_library(opm-json ${json_source})
target_link_libraries( opm-json ${Boost_LIBRARIES} )

set( HEADER_FILES JsonObject.hpp cjson/cJSON.h )

install( TARGETS opm-json DESTINATION ${CMAKE_INSTALL_LIBDIR} )
install( FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/opm/json)

