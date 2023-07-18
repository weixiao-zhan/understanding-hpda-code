#include "common.h"
#include "netio.cpp"


int main(int argc, char *argv[])
{
    hpda::engine engine;

    from_net<NTO_distance_entry> fn("127.0.0.1", 9000);
    fn.set_engine(&engine);

    hpda::output::internal::memory_output_impl<NTO_distance_entry> checker(&fn);
    checker.set_engine(&engine);

    engine.run();

    std::cout << "size " << checker.values().size() << std::endl;
    for (auto v : checker.values())
    {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<distance>() << std::endl;
        std::cout << std::endl;
    }

}