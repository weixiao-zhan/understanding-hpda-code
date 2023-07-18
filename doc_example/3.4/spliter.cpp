#include "common.h"
#include "hpda/processor/transform/split.h"
#include "netio.cpp"

// starting with a csv file reader
class cvs_extractor : public hpda::extractor::internal::raw_data_impl<NTO_data_entry>
{
public:
    cvs_extractor(const std::string &filename)
        : hpda::extractor::internal::raw_data_impl<NTO_data_entry>()
    {
        file.open(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file");
        }
    }
    ~cvs_extractor()
    {
        file.close();
    }

    bool process() override
    {
        std::string line;
        if (std::getline(file, line))
        {
            std::istringstream s(line);

            std::vector<std::string> fields;
            std::string field;
            while (getline(s, field, ','))
            {
                fields.push_back(field);
            }

            NTO_loc_info one_loc_info;
            one_loc_info.set<longitude, latitude, timestamp>(
                std::stof(fields[1]), std::stof(fields[2]), std::stof(fields[3]));

            NTO_data_entry one_entry;
            one_entry.set<phone_number>(std::stoll(fields[0]));
            one_entry.set<loc_info>(one_loc_info);
            hpda::extractor::internal::raw_data_impl<NTO_data_entry>::add_data(one_entry);
        }
        return hpda::extractor::internal::raw_data_impl<NTO_data_entry>::process();
    }

private:
    std::ifstream file;
};

class hash_spliter : public hpda::processor::internal::split_impl<NTO_data_entry>
{
public:
    hash_spliter(hpda::internal::processor_with_output<NTO_data_entry> *upper_stream)
        : hpda::processor::internal::split_impl<NTO_data_entry>(upper_stream)
    {
    }

    bool process() override
    {
        if (m_streams.empty() || !base::has_input_value())
        {
            return false;
        }
        NTO_data_entry t = base::input_value();
        size_t hashValue = hashFunc(t.get<phone_number>());
        m_streams[hashValue % m_streams.size()]->add_data(t.make_copy());
        consume_input_value();
        return true;
    }

protected:
    std::hash<int> hashFunc;
};

int main(int argc, char *argv[])
{
    hpda::engine engine;

    cvs_extractor ce("../doc_example/3.4/data/phone_geo_time.csv");
    ce.set_engine(&engine);
    /*
    hpda::output::internal::memory_output_impl<NTO_data_entry> checker( &ce );
    engine.run();
    std::cout << checker.values().size() << std::endl;
    for (auto v : checker.values()) {
        std::cout << v.get<phone_number>() << std::endl;
    }
    */

    hash_spliter hs(&ce);
    hs.set_engine(&engine);

    /*
    hpda::output::internal::memory_output_impl<NTO_data_entry> checker1( hs.new_split_stream() );
    hpda::output::internal::memory_output_impl<NTO_data_entry> checker2( hs.new_split_stream() );
    engine.run();
    std::cout << "size" << checker1.values().size() << std::endl;
    for (auto v : checker1.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<loc_info>().get<timestamp>()  << std::endl;
    }
    std::cout << "size" << checker2.values().size() << std::endl;
    for (auto v : checker2.values()) {
        std::cout << v.get<phone_number>() << "|";
        std::cout << v.get<loc_info>().get<timestamp>()  << std::endl ;
    }
    */

    netout<NTO_data_entry> no(hs.new_split_stream());
    no.set_engine(&engine);

    engine.run();
    no.hpda_engine_complete();
    return 0;
}