#include "worker.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "usage: <binary> <worker_idx>";
        exit(0);
    }

    uint id = std::stoi(argv[1]);
    std::cout << splitter_worker_port_base+id << " -> " << worker_aggregator_port_base+id << std::endl;
    hpda::engine engine;

    from_net<NTO_data_entry> fn("127.0.0.1", splitter_worker_port_base+id);
    fn.set_engine(&engine);

    group_and_sort gs(&fn);
    gs.set_engine(&engine);

    cal_distance cd(&gs);
    cd.set_engine(&engine);

    max_n mn(&cd, 5);
    mn.set_engine(&engine);

    to_net<NTO_distance_entry> tn(&mn, "127.0.0.1", worker_aggregator_port_base+id);
    tn.set_engine(&engine);

    engine.run();
}
