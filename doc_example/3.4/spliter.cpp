#include "common.h"
#include "hpda/processor/transform/split.h"
#include <queue>

// starting with a csv file reader
class cvs_extractor : public hpda::extractor::internal::extractor_base<data_entry>
{
public:
    cvs_extractor(const std::string &filename)
        : m_index(-1)
    {
        file.open(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file");
        }
        read_file();
    }

    bool process()
    {
        if (m_data.empty())
        {
            return false;
        }
        m_index++;
        if (m_index >= static_cast<int>(m_data.size()))
        {
            return false;
        }
        return true;
    }

    data_entry output_value()
    {
        if (m_data.size() <= m_index)
        {
            throw std::runtime_error("no more data in raw_data ");
        }
        return m_data[m_index];
    }

    ~cvs_extractor()
    {
        file.close();
    }

private:
    std::ifstream file;
    int32_t m_index;
    std::vector<data_entry> m_data;

    void read_file()
    {
        std::string line;

        while (std::getline(file, line))
        {
            std::istringstream s(line);

            std::vector<std::string> fields;
            std::string field;
            while (getline(s, field, ','))
            {
                fields.push_back(field);
            }

            data_entry one_entry;
            one_entry.set<phone_number>(std::stoll(fields[0]));
            one_entry.set<longitude>(std::stof(fields[1]));
            one_entry.set<latitude>(std::stof(fields[2]));
            one_entry.set<timestamp>(std::stoll(fields[3]));

            m_data.push_back(one_entry);
        }
    }
};

class hash_spliter : public hpda::processor::internal::split_impl<data_entry>
{
public:
    hash_spliter(hpda::internal::processor_with_output<data_entry> *upper_stream)
        : hpda::processor::internal::split_impl<data_entry>(upper_stream)
    {    }

    virtual bool process()
    {
        if (m_streams.empty() || !base::has_input_value() )
        {
            return false;
        }
        auto t = base::input_value();
        size_t hashValue = hashFunc(t.get<phone_number>());

        m_streams[hashValue%m_streams.size()]->add_data(t.make_copy());
        consume_input_value();
        return true;
    }

protected:
    std::hash<int> hashFunc;
};

