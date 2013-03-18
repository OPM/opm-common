add_library(Parser Parser.cpp EclipseDeck.cpp)
add_library(Logger Logger.cpp)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(tests)

