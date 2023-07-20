#include "../common.h"

int main(int argc, char* argv[]) {
    hpda::engine engine;

    hpda::extractor::internal::raw_data_impl<NTO_data_entry> rd;
    rd.set_engine(&engine);

    for (int i = 0; i < 10; i++) {
        NTO_data_entry d;
        d.set<phone_number>(i);
        rd.add_data(d);
    }

    to_net<NTO_data_entry> no(&rd, "127.0.0.1", std::stoi(argv[1]));
    no.set_engine(&engine);

    engine.run();
}