#include <iostream>
#include <string>
#include "hpda/hpda.h"
#include "ff/util/ntobject.h"

define_nt(name, std::string)
define_nt(score, int)
typedef hpda::ntobject<name, score> data_item_t;

int main(int argc, char * argv[]){
    hpda::engine engine;
    hpda::extractor::raw_data<name, score> rd;
    rd.set_engine(&engine);

    std::string n;
    int s;
    int num;
    for (int i = 0; i < 3; i++) {
      data_item_t d;
      std::cout<<"input "<<i<<"th student name and score"<<std::endl;
      std::cin>>n;
      std::cin>>s;
      d.set<name, score>(n, s);
      rd.add_data(d);
    }

    hpda::processor::filter<name, score> f(&rd, [](const data_item_t &d) {
      return d.get<score>() >= 60;
    });
  
    hpda::output::memory_output<name, score> mo(&f);
  
    engine.run();
    for(auto v : mo.values()){
        std::cout<<v.get<name>()<<": "<<v.get<score>()<<std::endl;
    }
  
    return 0;
}