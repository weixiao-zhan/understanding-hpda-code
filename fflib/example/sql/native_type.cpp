
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

using namespace ff::mysql;
define_column(c1, column, uint64_t, "id");
define_column(xx, column, std::bitset<63>, "idx");
// define_column(yy, column, json_m, "MULTIPOINT(0 0, 20 20, 60 60)");
typedef std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::microseconds>
    mtime_point;

typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
    time_point;
define_column(tms, column, mtime_point, "mts");
define_column(ts, column, time_point, "ts");
define_column(gap, column, std::chrono::milliseconds, "gap");
define_column(ydate, column, ::ff::mysql::date, "mdate");
define_column(yyear, column, ::ff::mysql::year, "myear");

typedef ff::sql::table<ff::sql::mysql<ff::sql::cppconn>, mymeta, c1, xx, tms,
                       ts, gap, ydate, yyear>
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
  std::cout << "create table done" << std::endl;

  mytable::row_collection_type rows;

  mytable::row_collection_type::row_type t1;
  //#if 0
  mtime_point mn = std::chrono::time_point_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now());
  time_point n = std::chrono::time_point_cast<std::chrono::seconds>(mn);
  auto tgap = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::hours(120) + std::chrono::seconds(23) +
      std::chrono::milliseconds(100));
  auto tdate = std::chrono::time_point_cast<::ff::mysql::date::duration>(n);
  auto tyear = ff::mysql::year(1978);

  std::cout << "time_point is: " << n.time_since_epoch().count() << std::endl;
  std::cout << "time_point (microsecond) is: " << mn.time_since_epoch().count()
            << std::endl;
  std::cout << "gap is :" << tgap.count() << std::endl;
  t1.set<c1, xx, ts, tms, gap, ydate, yyear>(1, 0x56, n, mn, tgap, tdate,
                                             tyear);
  rows.push_back(std::move(t1));

  mytable::insert_or_replace_rows(&engine, rows);

  auto ret1 = mytable::select<c1, xx, ts, tms, gap, yyear>(&engine).eval();
  std::cout << "size: " << ret1.size() << std::endl;
  for (size_t i = 0; i < ret1.size(); ++i) {
    std::cout << ret1[i].get<c1>() << ", " << ret1[i].get<xx>() << ", "
              << ret1[i].get<ts>().time_since_epoch().count() << ", "
              << ret1[i].get<tms>().time_since_epoch().count() << ", "
              << ret1[i].get<gap>().count() << ", "
              << ret1[i].get<yyear>().data() << std::endl;
  }
  std::cout << "---------------" << std::endl;
  // mytable::select<c1, c2>(&engine).where(c1::eq(1)).eval();
  return 0;
}
