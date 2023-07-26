#include "pp_processor.h"

define_nt(field0, int);
typedef ff::util::ntobject<field0> smpl_ntobj;

bool comp(smpl_ntobj& a, smpl_ntobj& b){
    return a.get<field0>() < b.get<field0>();
}

template<>
class named_pp<smpl_ntobj>
{
std::string name;
static const size_t width = 16;
public:
    named_pp<smpl_ntobj>(std::string name)
    {
        this->name = name;
        if (name.length() < width) {
            this->name = name;
            this->name.insert(0, width - name.length(), ' ');
        }
    }
    std::string get_name() 
    {
        return name;
    }
    std::string pp(smpl_ntobj t)
    {
        return std::to_string(t.get<field0>());
    }
};

void trace1() {
    hpda::engine engine;

    raw_data_pp<smpl_ntobj> rd1("raw data 1"); // [0] [2] [4]
    rd1.set_engine(&engine);
    for(int i = 0; i < 6; i+=2) {
        smpl_ntobj t;
        t.set<field0>(i);
        rd1.add_data(t);
    }

    raw_data_pp<smpl_ntobj> rd2("raw data 2"); // [1]
    rd2.set_engine(&engine);
    for(int i = 1; i < 3; i+=2) {
        smpl_ntobj t;
        t.set<field0>(i);
        rd2.add_data(t);
    }

    concat_pp<smpl_ntobj> concat(&rd1);
    concat.add_upper_stream(&rd2);
    concat.set_engine(&engine);


    sort_pp<smpl_ntobj> st(&concat, comp);
    st.set_engine(&engine);

    engine.run();
}

void trace2() {
    hpda::engine engine;

    raw_data_pp<smpl_ntobj> rd; // [0] [1]
    rd.set_engine(&engine);
    for(int i = 0; i < 4; i+=1) {
        smpl_ntobj t;
        t.set<field0>(i);
        rd.add_data(t);
    }

    split_pp<smpl_ntobj> split(&rd);
    split.set_engine(&engine);

    memory_output_pp<smpl_ntobj> mo1(split.new_split_stream(), "mo1");
    mo1.set_engine(&engine);

    memory_output_pp<smpl_ntobj> mo2(split.new_split_stream(), "mo2");
    mo2.set_engine(&engine);

    engine.run();
}

int main() {
    std::cout << "trace 1:" << std::endl;
    trace1();
    std::cout << "trace 2:" << std::endl;
    trace2();
}
