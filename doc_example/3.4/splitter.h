#pragma once
#include "common.h"
#include "hpda/processor/transform/split.h"
#include "netio.cpp"

// starting with a csv file reader
class cvs_extractor : public hpda::internal::processor_with_output<NTO_data_entry>
{
public:
    cvs_extractor(const std::string &filename)
        : hpda::internal::processor_with_output<NTO_data_entry>()
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
        if (std::getline(file, line)) {
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

            theObj.set<phone_number>(std::stoll(fields[0]));
            theObj.set<loc_info>(one_loc_info);
            return true;
        }
        return false;
    }

    NTO_data_entry output_value() override 
    {
        return theObj;
    }

private:
    std::ifstream file;
    NTO_data_entry theObj;
};

class hash_splitter : public hpda::processor::internal::split_impl<NTO_data_entry>
{
public:
    hash_splitter(hpda::internal::processor_with_output<NTO_data_entry> *upper_stream)
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
