
add_executable(thor-tool thor.c)

target_link_libraries(thor-tool thor)

set_target_properties(thor-tool PROPERTIES OUTPUT_NAME "thor")
install(TARGETS thor-tool DESTINATION ${CMAKE_INSTALL_BINDIR} EXPORT ThorTargets)


