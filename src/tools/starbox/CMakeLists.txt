add_executable(starbox starbox.c)
target_link_libraries(starbox plcore)
set_target_properties(starbox PROPERTIES FOLDER "Tools")

file(COPY starbox_license.txt DESTINATION ${CMAKE_HOME_DIRECTORY}/release/legal/)
