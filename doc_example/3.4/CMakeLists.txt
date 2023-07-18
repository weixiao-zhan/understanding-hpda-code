add_executable(spliter spliter.cpp)
target_link_libraries(spliter hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(worker worker.cpp)
target_link_libraries(worker hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(netin netin_main.cpp)
target_link_libraries(netin hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(netout netout_main.cpp)
target_link_libraries(netout hpda ff_net glog boost_program_options boost_system -lpthread)