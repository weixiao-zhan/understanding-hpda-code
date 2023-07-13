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
#include "ff/sql/mysql.hpp"
#include "ff/sql/table.h"
#include <chrono>
#include <cstdlib>
#include <iostream>

struct mymeta {
  constexpr static const char *table_name = "test_transaction";
};

define_column(c1, key, uint64_t, "id");
define_column(c2, column, std::string, "event");
define_column(c3, index, uint64_t, "ts");

typedef ff::sql::table<ff::sql::mysql<ff::sql::cppconn>, mymeta, c1, c2, c3>
    mytable;

int main(int argc, char *argv[]) {

  ff::sql::mysql<ff::sql::cppconn> engine("tcp://127.0.0.1:3306", "root", "",
                                          "test");
  mytable::delete_rows(&engine);
  mytable::create_table(&engine);

  mytable::row_collection_type rows;

  mytable::row_collection_type::row_type t1;
  int64_t max = 10000;
  for (int i = 0; i < max; ++i) {
    t1.set<c1, c2, c3>(i, std::to_string(i), i);
    rows.push_back(t1.make_copy());
  }

  auto start = std::chrono::system_clock::now();
  mytable::insert_or_replace_rows(&engine, rows);
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  std::cout << "items: " << max << ", time: " << elapsed_seconds.count()
            << " seconds" << std::endl;
  ;

  return 0;
}
