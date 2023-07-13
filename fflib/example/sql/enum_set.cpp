#include "ff/sql/mysql.hpp"
#include "ff/sql/table.h"

struct mymeta {
  constexpr static const char *table_name = "xxxyyy_enum_set";
};
char enum_h1[] = "male";
char enum_h2[] = "female";
char enum_bi[] = "bisex";
char enum_none[] = "none";
typedef ff::mysql::enum_m<enum_h1, enum_h2, enum_bi, enum_none> sex_enum;
typedef ff::mysql::set_m<enum_h1, enum_h2, enum_bi, enum_none> sex_set;

define_column(c1, column, uint64_t, "id");
define_column(c2, key, ff::mysql::varchar_m<64>, "event");
define_column(sex, column, sex_enum, "sex");
define_column(msex, column, sex_set, "msex");

typedef ff::sql::table<ff::sql::mysql<ff::sql::cppconn>, mymeta, c1, c2, sex,
                       msex>
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
  sex_enum se("male");
  sex_set sset({"bisex", "male"});
  auto vs = sset.value();
  t1.set<c1, c2, sex, msex>(1, "1992-08-07 13:05:01", se, sset);
  rows.push_back(std::move(t1));

  mytable::insert_or_replace_rows(&engine, rows);

  auto ret1 = mytable::select<c1, c2, sex, msex>(&engine).eval();
  std::cout << "size: " << ret1.size() << std::endl;
  for (size_t i = 0; i < ret1.size(); ++i) {
    std::cout << ret1[i].get<c1>() << ", " << ret1[i].get<c2>() << ", "
              << ret1[i].get<sex>() << std::endl;
    auto mss = ret1[i].get<msex>().value();
    std::cout << "msex: ";
    for (auto is : mss) {
      std::cout << is << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "---------------" << std::endl;
  mytable::select<c1, c2>(&engine).where(c1::eq(1)).eval();

  return 0;
}
