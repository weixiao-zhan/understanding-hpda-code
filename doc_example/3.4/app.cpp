#include "spliter.cpp"
#include <iostream>

int main()
{
    hpda::engine engine;

    cvs_extractor ce("../doc_example/3.4/data/phone_geo_time.csv");
    ce.set_engine(&engine);
/*
    hpda::output::internal::memory_output_impl<data_entry> checker( &ce );
    engine.run();
    std::cout << checker.values().size() << std::endl;
    for (auto v : checker.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<timestamp>()  << std::endl;
    }
*/

    hash_spliter hs(&ce);
    hs.set_engine(&engine);
    
    hpda::output::internal::memory_output_impl<data_entry> checker1( hs.new_split_stream() );
    hpda::output::internal::memory_output_impl<data_entry> checker2( hs.new_split_stream() );

    engine.run();

/*
    std::cout << "size" << checker1.values().size() << std::endl;
    for (auto v : checker1.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<timestamp>()  << std::endl;
    }
    std::cout << "size" << checker2.values().size() << std::endl;
    for (auto v : checker2.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<timestamp>()  << std::endl ;
    }
*/
}