#include"netio.cpp"
#include"common.h"

int main() {
    hpda::engine engine;
    netin<NTO_data_entry> ni;
    ni.set_engine(&engine);

    hpda::output::internal::memory_output_impl<NTO_data_entry> mo(&ni);
    ni.set_engine(&engine);

    engine.run();

    std::cout << mo.values().size() <<std::endl;
    for(auto e : mo.values()) {
        std::cout << e.get<phone_number>() << std::endl;
    }
}