#include "aggregator.h"
#include "worker.h"

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

    max_n mn(concat, 5);
    mn.set_engine(&engine);

    write_to_file wf(&mn, "../doc_example/3.4/data/output.csv");
    wf.set_engine(&engine);

    engine.run();    
}