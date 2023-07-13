/***********************************************
  The MIT License (MIT)

  Copyright (c) 2012 Athrun Arthur <athrunarthur@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
 *************************************************/
#include "ff/net/middleware/ntpackage.h"
#include <cstdint>
#include <iostream>
#include <vector>

// extend an arbitrary type
struct mydata {
  uint32_t v1;
  uint32_t v2;
};
namespace ff {
namespace net {
template <> class archive_helper<mydata> {
public:
  static uint32_t serialize(char *buf, const mydata &d, size_t len) {
    memcpy(buf, (const char *)&d, sizeof(d));
    return sizeof(d);
  }
  static uint32_t deserialize(const char *buf, mydata &d, size_t len) {
    memcpy((char *)&d, buf, sizeof(d));
    return sizeof(d);
  }
  static uint32_t length(mydata &d) { return sizeof(d); }
};
} // namespace net
} // namespace ff

define_nt(email, std::string, "email");
define_nt(uid, uint64_t, "uid");
define_nt(name, std::string, "name");
define_nt(type, std::string);
define_nt(ns, std::vector<std::string>, "ns");

define_nt(data, mydata, "md");

typedef ff::net::ntpackage<112, email, uid, name, ns, data> mypackage;

int main(int, char *[]) {
  mypackage p1;
  std::vector<std::string> nss;
  nss.push_back("n1");
  nss.push_back("n2");
  nss.push_back("n3");
  p1.set<email, uid, name>("xuepeng", 123, "xuepeng@email.com");
  p1.set<ns>(nss);

  ff::net::marshaler c(ff::net::marshaler::length_retriver);
  p1.arch(c);
  size_t l = c.get_length();
  std::cout << "length: " << l << std::endl;
  char *buf = new char[l];
  ff::net::marshaler sc(buf, l, ff::net::marshaler::serializer);
  p1.arch(sc);

  mypackage p2;
  ff::net::marshaler dc(buf, l, ff::net::marshaler::deserializer);
  p2.arch(dc);

  std::cout << "email: " << p2.get<email>() << std::endl;
  std::cout << "uid: " << p2.get<uid>() << std::endl;
  std::cout << "_name: " << p2.get<name>() << std::endl;
  auto tss = p2.get<ns>();
  for (size_t i = 0; i < tss.size(); ++i) {
    std::cout << "\tns: " << tss[i] << std::endl;
  }

  return 0;
}
