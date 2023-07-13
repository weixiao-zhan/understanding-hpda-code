#include "ff/sql/mysql.hpp"
#include "ff/sql/table.h"

struct mymeta {
  constexpr static const char *table_name = "xxxyyy_blob";
};

define_column(c1, column, uint64_t, "id");
define_column(c2, key, ff::mysql::varchar_m<64>, "event");
define_column(msex, column, ff::mysql::blob, "msex");
define_column(msex2, column, ff::mysql::tiny_blob, "msex2");

typedef ff::sql::table<ff::sql::mysql<ff::sql::cppconn>, mymeta, c1, c2, msex,
                       msex2>
    mytable;

void output_stream(std::istream *stream) {
  auto is = stream;
  while (!is->eof()) {
    if (is->bad()) {
      std::cout << "input stream corrupted" << std::endl;
      break;
    }
    if (is->fail()) {
      std::cout << "input stream failed" << std::endl;
      is->clear(std::istream::failbit);
      break;
    }
    std::string str;
    std::getline(*is, str);
    std::cout << "content is : " << str << std::endl;
  }
}
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
  std::stringstream sstream;
  sstream << "hello world";
  ff::mysql::blob bm(&sstream);

  std::stringstream sstream2;
  sstream2 << "hello world 2";
  ff::mysql::tiny_blob bm2(&sstream2);
  t1.set<c1, c2, msex, msex2>(1, "1992-08-07 13:05:01", bm, bm2);
  rows.push_back(std::move(t1));

  mytable::insert_or_replace_rows(&engine, rows);

  auto ret1 = mytable::select<c1, c2, msex, msex2>(&engine).eval();
  std::cout << "size: " << ret1.size() << std::endl;
  for (size_t i = 0; i < ret1.size(); ++i) {
    std::cout << ret1[i].get<c1>() << ", " << ret1[i].get<c2>() << ", "
              << std::endl;

    output_stream(ret1[i].get<msex>().stream());
    output_stream(ret1[i].get<msex2>().stream());
  }
  std::cout << "---------------" << std::endl;
  mytable::select<c1, c2>(&engine).where(c1::eq(1)).eval();

  return 0;
}
