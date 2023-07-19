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
    std::vector<hpda::output::internal::memory_output_impl<NTO_distance_entry>*> checker_list;

    for(uint i = 0; i < worker_num; i++) {
        from_net<NTO_distance_entry>* tmp = new from_net<NTO_distance_entry>("127.0.0.1", 9000+i);
        tmp->set_engine(&engine);
        input_list.push_back(tmp);

        auto* checker = new hpda::output::internal::memory_output_impl<NTO_distance_entry>(tmp);
        checker->set_engine(&engine);
        checker_list.push_back(checker);
    }

    engine.run();

    for (auto checker : checker_list) {
        std::cout << "size " << checker->values().size() << std::endl;
        for (auto v : checker->values())
        {
            std::cout << v.get<phone_number>() << "|";
            std::cout << v.get<distance>() << std::endl;
            std::cout << std::endl;
        }
    }
}