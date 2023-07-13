//g++ -I/opt/ypc/include/ nt_set.cpp

#include "ff/util/ntobject.h"
#include "hpda/hpda.h"
#include <iostream>

enum gender_t {
    male,
    female
};

define_nt(name,std::string);
define_nt(id, std::string);
define_nt(gender, gender_t);
define_nt(birthday, std::string);
typedef ff::util::ntobject<name, id, gender, birthday> student;

define_nt(friends, std::vector<std::string>);
define_nt(classmates, std::vector<student>)

define_nt(address, std::string);
typedef ff::util::ntobject<name, id, gender, address> ya_student;

void basic_example() {
    std::cout << "-----basic_example-----" << std::endl;

    student t;
    t.set<name>("Bob");
    std::cout<<t.get<name>()<<std::endl;

    t.set<name, gender, id>("Bob", male, "2023-001");
    std::cout<<t.get<id>()<<std::endl;

    ya_student ya;
    ya.set<name, id, gender, address>("Alice", "2023-002", female, "beijing");
    t = ya;
    std::cout<<t.get<id>()<<std::endl;
}

/*
//template <int Index>
template <int Index, typename NT>
struct traverse_helper : public ff::util::ntobject<ARGS ... >
{
public:
    traverse_helper() 
        : ff::util::ntobject<ARGS ...>()
    {}

    template <typename T>
    static auto traverse_on(T &&t) -> typename std::enable_if<(std::remove_reference<T>::type::type_list::len > Index), void>::type
    {
        // do anything
        std::cout << std::get<Index>(*t.m_content) << std ::endl;
        traverse_helper<Index + 1>::traverse_on(std::forward(t));
    }

    template <typename T>
    static auto traverse_on(T &&t) 
        -> typename std::enable_if<(std::remove_reference<T>::type::type_list::len <= Index), void>::type
    {}
};

typedef traverse_helper<0, name, id, gender, birthday> student_trv;

template <typename NT>
void traverse(const NT &obj)
{
    //traverse_helper<0, NT::type_list>::traverse_on(obj);
}
*/

template <int Index>
struct traverse_helper
{
    template <typename T>
    static auto on(T &&t) 
        -> typename std::enable_if<(std::remove_reference<T>::type::type_list::len > Index), void>::type
    {
        // do anything
        std::cout << std::get<Index>(*(t.get_m_content())) << std ::endl;
        traverse_helper<Index + 1>::on(t);
    }
    template <typename T>
    static auto on(T &&t) -> typename std::enable_if<(std::remove_reference<T>::type::type_list::len <= Index), void>::type {}
};
template <typename NT>
void traverse(NT &obj)
{
    traverse_helper<0>::on(obj);
}

void traverse_example() {
    std::cout << "-----traverse_example-----" << std::endl;
    student y;
    y.set<name, id, gender, birthday>("Alice", "2023-002", female, "1970-1-1");
    traverse(y);
}


int main()
{
    basic_example();
    traverse_example();
}

