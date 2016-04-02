add_executable(OpmLoadDeck OpmLoadDeck)
target_link_libraries(OpmLoadDeck opmparser)
install(TARGETS OpmLoadDeck DESTINATION "bin")

