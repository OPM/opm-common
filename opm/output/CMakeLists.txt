set( SOURCE_FILES
Output.cpp
)

set( HEADER_FILES
Output.hpp
)

add_library(opmoutput ${SOURCE_FILES})
target_link_libraries(opmoutput ${ERT_LIBRARIES})
set_target_properties(opmoutput PROPERTIES VERSION ${opm-output_VERSION_MAJOR}.${opm-output_VERSION_MINOR}
                                           SOVERSION ${opm-output_VERSION_MAJOR})

include(${PROJECT_SOURCE_DIR}/cmake/Modules/install_headers.cmake)   
install_headers("${HEADER_FILES}" "${CMAKE_INSTALL_PREFIX}")
install(TARGETS opmoutput DESTINATION ${CMAKE_INSTALL_LIBDIR})
