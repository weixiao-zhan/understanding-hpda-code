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
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <string>

typedef std::array<char, 1024 * 1024> uname_t;
define_nt(email, std::string, "email");
define_nt(uid0, uint64_t, "ui0");
define_nt(uid1, uint64_t, "ui1");
define_nt(uid2, uint64_t, "ui2");
define_nt(uid3, uint64_t, "ui3");
define_nt(uname, uname_t, "uname");

typedef ff::util::ntobject<uid0, email, uname, uid1, uid2, uid3> data_item_t;
typedef ff::util::ntcompact_array<data_item_t> data_array_t;


void print_elapsed_seconds(
    const std::string &prefix,
    std::chrono::time_point<std::chrono::system_clock> &start) {
  auto end = std::chrono::system_clock::now();
  auto elapsed_seconds =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << prefix << ", " << elapsed_seconds.count() << std::endl;
  start = std::chrono::system_clock::now();
}

int main(int argc, char *argv[]) {
  std::chrono::time_point<std::chrono::system_clock> start, end;

  int max = 10000;
  int max_loop = 1000000;
  typedef ff::util::ntarray<uid0, email, uname, uid1, uid2, uid3> naive_array_t;
  naive_array_t naive_array;
  std::cout << "row size: " << sizeof(naive_array_t::row_type) << std::endl;
  std::cout << "data size: " << sizeof(naive_array_t::row_type::content_type)
            << std::endl;
  data_array_t array;

  std::vector<int> acc;
  for (int i = 0; i < max; ++i) {
    acc.push_back(rand() % max);
    // acc.push_back(i);
  }

  start = std::chrono::system_clock::now();
  for (int i = 0; i < max; ++i) {
    data_item_t di;
    di.set<uid0, email, uname, uid1, uid2, uid3>(i, std::to_string(i),
                                                 uname_t{'x'}, i, i, i);
    naive_array.push_back(di);
  }
  print_elapsed_seconds("ntarray insert time: ", start);

  for (int i = 0; i < max; ++i) {
    array.push_back<uid0, email, uname, uid1, uid2, uid3>(
        i, std::to_string(i), uname_t{'x'}, i, i, i);
  }
  print_elapsed_seconds("ntcompact_array insert time: ", start);

  uint64_t sum = 0;
  for (int j = 0; j < max_loop; ++j) {
    for (size_t i = 0; i < naive_array.size(); ++i) {
      sum += naive_array[acc[i]].get<uid0>();
    }
  }
  print_elapsed_seconds("naive_array iterate time: ", start);

  sum = 0;
  for (int j = 0; j < max_loop; ++j) {
    for (size_t i = 0; i < array.size(); ++i) {
      sum += array[acc[i]].get<uid0>();
    }
  }
  print_elapsed_seconds("ntcompact_array iterate time: ", start);

  auto kt =
      array.reshape<ff::util::nt_collect<email>, ff::util::nt_collect<uid0>>();

  start = std::chrono::system_clock::now();
  sum = 0;
  for (int j = 0; j < max_loop; ++j) {
    for (size_t i = 0; i < kt.size(); ++i) {
      sum += kt[acc[i]].get<uid0>();
    }
  }
  print_elapsed_seconds("ntcompact_array after reshap iterate time: ", start);

  sum = 0;
  auto &tuid = kt.collect<ff::util::nt_collect<uid0>>();
  for (int j = 0; j < max_loop; ++j) {
    for (size_t i = 0; i < tuid.size(); ++i) {
      sum += tuid[acc[i]];
    }
  }
  print_elapsed_seconds("ntcompact_array direct access iterate time: ", start);
  char c;
  std::cin >> c;
  return 0;
}
