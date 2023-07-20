#include "splitter.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "usage: <binary> <num_worker>";
        exit(0);
    }
    hpda::engine engine;

    cvs_extractor ce("../doc_example/3.4/data/phone_geo_time.csv");
    ce.set_engine(&engine);

    hash_splitter hs(&ce);
    hs.set_engine(&engine);

    uint worker_num = std::stoi(argv[1]);
    std::vector<to_net<NTO_data_entry>*> output_list;
    for(int i = 0; i < worker_num; i++) {
        to_net<NTO_data_entry>* tmp = new to_net<NTO_data_entry>(hs.new_split_stream(), "127.0.0.1", splitter_worker_port_base+i);
        tmp->set_engine(&engine);
        output_list.push_back(tmp);
    }

    engine.run();

    for(int i = 0; i < worker_num; i++) {
        output_list[i]->end_net_module();        
    }

    return 0;
}