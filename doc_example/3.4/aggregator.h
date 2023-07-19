#pragma once
#include "common.h"

class write_to_file : public hpda::internal::processor_with_input<NTO_distance_entry>
{
public:
    write_to_file(hpda::internal::processor_with_output<NTO_distance_entry>* upper_stream, std::string filename)
        : hpda::internal::processor_with_input<NTO_distance_entry>(upper_stream)
    {
        file.open(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file");
        }
    }

    ~write_to_file()
    {
        file.close();
    }

    bool process() override
    {
        if (has_input_value()) {
            file << input_value().get<phone_number>() << ", " << input_value().get<distance>() << "\n";
            consume_input_value();
            return true;
        }
        return false;
    }

private:
    std::ofstream file;
};

