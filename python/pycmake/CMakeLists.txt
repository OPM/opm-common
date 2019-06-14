cmake_minimum_required( VERSION 2.0 )
project( pycmake )

if(PYCMAKE_ENABLE_TESTING)
    add_subdirectory(test)
endif()

install(FILES FindPythonModule.cmake
              PythonPackage.cmake
        DESTINATION share/cmake/Modules)
