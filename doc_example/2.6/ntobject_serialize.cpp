#include "ff/net/middleware/ntpackage.h"
#include <cstdint>
#include <iostream>
#include <vector>

// extend an arbitrary type
struct mydata_t
{
    uint32_t v1;
    uint32_t v2;
};
define_nt(name, std::string, "name");
define_nt(uid, uint64_t, "uid");
define_nt(email, std::string, "email");
define_nt(my_type, std::string);
define_nt(my_strings, std::vector<std::string>, "strings");
define_nt(my_data, mydata_t, "mydata");

typedef ff::net::ntpackage<112, name, uid, email, my_type, my_strings, my_data> mypackage;

namespace ff
{
    namespace net
    {
        template <>
        class archive_helper<mydata_t>
        {
        public:
            static uint32_t serialize(char *buf, const mydata_t &d, size_t len)
            {
                memcpy(buf, (const char *)&d, sizeof(d));
                return sizeof(d);
            }
            static uint32_t deserialize(const char *buf, mydata_t &d, size_t len)
            {
                memcpy((char *)&d, buf, sizeof(d));
                return sizeof(d);
            }
            static uint32_t length(mydata_t &d) { return sizeof(d); }
        };
    } // namespace net
} // namespace ff


int main()
{
    mypackage orig_pkg;
    orig_pkg.set<name, uid, email>("xuepeng", 123, "xuepeng@email.com");
    std::vector<std::string> strs;
    strs.push_back("n1");
    strs.push_back("n2");
    strs.push_back("n3");
    orig_pkg.set<my_strings>(strs);

    ff::net::marshaler length_retriver(ff::net::marshaler::length_retriver);
    orig_pkg.arch(length_retriver);
    size_t l = length_retriver.get_length();
    std::cout << "length: " << l << std::endl;

    char *buf = new char[l];
    ff::net::marshaler encoder(buf, l, ff::net::marshaler::serializer);
    orig_pkg.arch(encoder);

    mypackage new_pkg;
    ff::net::marshaler decoder(buf, l, ff::net::marshaler::deserializer);
    new_pkg.arch(decoder);

    std::cout << "email: " << new_pkg.get<email>() << std::endl;
    std::cout << "uid: " << new_pkg.get<uid>() << std::endl;
    std::cout << "_name: " << new_pkg.get<name>() << std::endl;
    auto new_strs = new_pkg.get<my_strings>();
    for (auto str : new_strs)
    {
        std::cout << str << std::endl;
    }
    return 0;
}