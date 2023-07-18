add_executable(spliter spliter.cpp)
target_link_libraries(spliter hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(worker worker.cpp)
target_link_libraries(worker hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(from_net from_net_main.cpp)
target_link_libraries(from_net hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(to_net to_net_main.cpp)
target_link_libraries(to_net hpda ff_net glog boost_program_options boost_system -lpthread)