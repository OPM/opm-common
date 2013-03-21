
add_library(Parser Parser.cpp data/KeywordRecordSet.cpp)
add_library(Logger Logger.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

