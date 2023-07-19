#include "common.h"
#include "netio.cpp"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "usage: <binary> <worker_num>";
        exit(0);
    }
    hpda::engine engine;
    
    uint worker_num = std::stoi(argv[1]);
    if (worker_num < 1) {
        std::cerr << "at least one worker" << std::endl;
        exit(0);
    }

    std::vector<from_net<NTO_distance_entry>*> input_list;
    hpda::processor::internal::concat_impl<NTO_distance_entry>* concat;

    std::vector<hpda::output::internal::memory_output_impl<NTO_distance_entry>*> checker_list;
    for(uint i = 0; i < worker_num; i++) {
        from_net<NTO_distance_entry>* tmp = new from_net<NTO_distance_entry>("127.0.0.1", worker_aggregator_port_base+i);
        tmp->set_engine(&engine);
        tmp->start_net_app();
        input_list.push_back(tmp);

        if (i == 0) {
            concat = new hpda::processor::internal::concat_impl<NTO_distance_entry>(tmp);
            concat->set_engine(&engine);
        } else {
            concat->add_upper_stream(tmp);
        }
    }

    auto* checker = new hpda::output::internal::memory_output_impl<NTO_distance_entry>(concat);
    checker->set_engine(&engine);

    engine.run();

    std::cout << "size " << checker->values().size() << std::endl;
    for (auto v : checker->values())
    {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<distance>() << std::endl;
        std::cout << std::endl;
    }
    
}