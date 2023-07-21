#include "../common.h"

int main(int argc, char* argv[]) {
    uint port;
    if (argc == 1){
        port = 8000;
    } else {
        port = std::stoi(argv[1]);
    }
    hpda::engine engine;
    from_net<NTO_distance_entry> ni("127.0.0.1", port);
    ni.set_engine(&engine);

    hpda::output::internal::memory_output_impl<NTO_distance_entry> mo(&ni);
    ni.set_engine(&engine);

    engine.run();

    std::cout << mo.values().size() <<std::endl;
    for(auto e : mo.values()) {
        std::cout << e.get<phone_number>() << std::endl;
    }
}