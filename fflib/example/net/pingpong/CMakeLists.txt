add_executable(pingpong main.cpp)

target_link_libraries(pingpong ff_net boost_program_options boost_system -lpthread)
