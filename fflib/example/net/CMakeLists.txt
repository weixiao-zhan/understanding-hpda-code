add_executable(def_package def_package.cpp)

target_link_libraries(def_package ff_net boost_program_options boost_system)

add_subdirectory(pingpong)
