add_executable(splitter splitter.cpp)
target_link_libraries(splitter hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(worker worker.cpp)
target_link_libraries(worker hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(aggregater aggregater.cpp)
target_link_libraries(aggregater hpda ff_net glog boost_program_options boost_system -lpthread)

add_subdirectory(test)