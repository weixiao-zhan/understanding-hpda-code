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

struct mymeta {
  constexpr static const char *table_name = "xxxyyy";
};

struct md {
  int a;
  int b;
  int c;
};

typedef ff::mysql::decimal<35, 5> mydecimal_t;

define_column(c1, column, uint64_t, "id");
define_column(c2, column, std::string, "event");
define_column(c3, index, uint64_t, "ts");
define_column(c4, index, int64_t, "a");
define_column(c5, column, uint32_t, "b");
define_column(c6, column, int32_t, "c");
define_column(c7, column, double, "d");
define_column(c8, column, float, "e");
// define_column(c7, column, int, "d");
// define_column(c8, column, int, "e");
define_column(c9, column, uint16_t, "u16");
define_column(c10, column, int16_t, "i16");
define_column(c11, column, uint8_t, "u8");
define_column(c12, column, int8_t, "i8");
define_column(c13, column, md, "md");
define_column(c14, column, mydecimal_t, "c14");
define_column(c15, column, ff::mysql::medium_int, "c15");
// define_column(c16, column, ff::mysql::blob, "blob");
// define_column(c14, column, std::istream*, "istream");

typedef ff::sql::table<ff::sql::mysql<ff::sql::cppconn>, mymeta, c1, c2, c3, c4,
                       c5, c6, c7, c8, c9, c10, c11, c12, c14, c15>
    mytable;

int main(int argc, char *argv[]) {

  ff::sql::mysql<ff::sql::cppconn> engine("tcp://127.0.0.1:3306", "test",
                                          "123456", "testdb");
  try {
    mytable::drop_table(&engine);
  } catch (...) {
    std::cout << "drop_table failed" << std::endl;
  }
  mytable::create_table(&engine);

  mytable::row_collection_type rows;

  mytable::row_collection_type::row_type t1;
  mydecimal_t mt("12345.24456789");
  mt *= 2;
  t1.set<c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c14, c15>(
      1, "1992-08-07 13:05:01", 123435, 1, 1, 1, 3.2, 2.0, 16, 16.0, 8, 8.0, mt,
      15);
  rows.push_back(std::move(t1));

  mytable::insert_or_replace_rows(&engine, rows);

  auto ret1 = mytable::select<c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12,
                              c14, c15>(&engine)
                  .eval();
  std::cout << "size: " << ret1.size() << std::endl;
  for (size_t i = 0; i < ret1.size(); ++i) {
    std::cout << ret1[i].get<c1>() << ", " << ret1[i].get<c2>() << ", "
              << ret1[i].get<c3>() << ", " << ret1[i].get<c4>() << ", "
              << ret1[i].get<c5>() << ", " << ret1[i].get<c6>() << ", "
              << ret1[i].get<c7>() << ", " << ret1[i].get<c8>() << ", "
              << ret1[i].get<c9>() << ", " << ret1[i].get<c10>() << ", "
              << static_cast<int>(ret1[i].get<c11>()) << ", "
              << static_cast<int>(ret1[i].get<c12>()) << ", "
              << ret1[i].get<c14>() << ", " << ret1[i].get<c15>().data()
              << std::endl;
  }
  std::cout << "---------------" << std::endl;
  mytable::select<c1, c2>(&engine).where(c1::eq(1)).eval();

  return 0;
}
