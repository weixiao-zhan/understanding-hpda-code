add_executable(splitter splitter.cpp)
target_link_libraries(splitter hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(worker worker.cpp)
target_link_libraries(worker hpda ff_net glog boost_program_options boost_system -lpthread)

add_executable(aggregator aggregator.cpp)
target_link_libraries(aggregator hpda ff_net glog boost_program_options boost_system -lpthread)

add_subdirectory(test)