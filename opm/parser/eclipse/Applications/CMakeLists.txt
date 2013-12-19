add_executable(eclipsedatadoctor EclipseDataDoctor.cpp)
target_link_libraries(eclipsedatadoctor Parser)

add_executable(keyworddescriber KeywordDescriber.cpp)
target_link_libraries(keyworddescriber Parser)

add_executable(schedule Schedule.cpp)
target_link_libraries(schedule Parser)

