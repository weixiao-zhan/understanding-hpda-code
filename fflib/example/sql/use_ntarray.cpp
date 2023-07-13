/***********************************************
  The MIT License (MIT)

  Copyright (c) 2012-2020 Athrun Arthur <athrunarthur@gmail.com>

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
#include "ff/util/ntcompact_array.h"
#include "ff/util/ntobject.h"
#include <iostream>
#include <string>

define_nt(email, std::string, "email");
define_nt(uid, uint64_t, "ui");
define_nt(uname, std::string, "uname");

typedef ff::util::ntobject<email, uid, uname> data_item_t;
typedef ff::util::ntcompact_array<data_item_t> data_array_t;

int main(int argc, char *argv[]) {
  data_array_t array;

  // insert with template push_back
  for (int i = 0; i < 10; ++i) {
    array.push_back<email, uid, uname>(std::to_string(i) + "@mail.com", i,
                                       std::to_string(i) + "_name");
  }

  // iterate with operator[]
  for (size_t i = 0; i < array.size(); ++i) {
    std::cout << array[i].get<email>() << ", " << array[i].get<uid>() << ", "
              << array[i].get<uname>() << std::endl;
  }

  std::cout << "size: " << array.size() << std::endl;
  // iterate with iterator
  for (auto it = array.begin(); it != array.end(); it++) {
    std::cout << (*it).get<email>() << ", " << (*it).get<uid>() << ", "
              << (*it).get<uname>() << std::endl;
  }

  auto kt = array.reshape<ff::util::nt_collect<uid, uname>>();
  std::cout << "after reshape size: " << kt.size();
  for (auto it = kt.begin(); it != kt.end(); it++) {
    std::cout << (*it).get<uid>() << ", " << (*it).get<uname>() << std::endl;
  }

  return 0;
}
