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
#include <thread>

struct mymeta {
  constexpr static const char *table_name = "yyy";
};

define_column(c1, column, uint64_t, "id");
define_column(c2, key, std::string, "event");
define_column(c3, index, uint64_t, "ts");

typedef ff::sql::table<ff::sql::mysql<ff::sql::cppconn>, mymeta, c1, c2, c3>
    mytable;

int main(int argc, char *argv[]) {

  ff::sql::mysql<ff::sql::cppconn> engine("tcp://127.0.0.1:3306", "root", "",
                                          "test");
  mytable::create_table(&engine);

  mytable::row_collection_type rows;

  mytable::row_collection_type::row_type t1, t2;
  t1.set<c1, c2, c3>(1, "c1", 123435);
  rows.push_back(std::move(t1));
  t2.set<c1, c2, c3>(2, "c2", 1235);
  rows.push_back(std::move(t2));

  mytable::insert_or_replace_rows(&engine, rows);

  std::thread thrd([&engine]() {
    auto local_engine = engine.thread_copy();
    auto ret2 = mytable::select<c1, c2, c3>(local_engine.get())
                    // auto ret2 = mytable::select<c1, c2, c3>(&engine)
                    .where(c1::eq(2))
                    .order_by<c1, ff::sql::desc>()
                    .limit(1)
                    .eval();
    std::cout << ret2.size() << std::endl;
    std::cout << ret2[0].get<c1>() << ", " << ret2[0].get<c2>() << ", "
              << ret2[0].get<c3>() << std::endl;
  });

  auto ret1 = mytable::select<c1, c2, c3>(&engine).eval();
  std::cout << "size: " << ret1.size() << std::endl;
  for (size_t i = 0; i < ret1.size(); ++i) {
    std::cout << ret1[i].get<c1>() << ", " << ret1[i].get<c2>() << ", "
              << ret1[i].get<c3>() << std::endl;
  }

  thrd.join();

  return 0;
}
