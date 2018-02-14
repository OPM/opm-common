pybind11_add_module(libsunbeam sunbeam.cpp
                               eclipse_state.cpp
                               deck_keyword.cpp
                               deck.cpp
                               well.cpp
                               sunbeam_state.cpp
                               schedule.cpp
                               completion.cpp
                               eclipse_config.cpp
                               table_manager.cpp
                               eclipse_grid.cpp
                               group.cpp
                               group_tree.cpp
                               eclipse_3d_properties.cpp
                               parser.cpp)

set_target_properties( libsunbeam PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/python/sunbeam )
target_link_libraries( libsunbeam PRIVATE ${opm-parser_LIBRARIES} )
install(TARGETS libsunbeam DESTINATION ${PYTHON_INSTALL_PREFIX}/sunbeam)
