#include "ff/sql/mysql.hpp"
#include "ff/sql/table.h"

struct mymeta
{
    constexpr static const char *table_name = "yyy";
};

define_column(c1, column, uint64_t, "id");
define_column(c2, key, std::string, "event");
define_column(c3, index, uint64_t, "ts");

typedef ff::sql::default_table<mymeta, c1, c2, c3> mytable;

int main(int argc, char *argv[])
{
    ff::sql::default_engine engine(CONNECTION, USER_NAME, PASSWORD, DB_NAME);
    mytable::create_table(&engine);

    mytable::row_collection_type rows;

    mytable::row_collection_type::row_type t1, t2;
    t1.set<c1, c2, c3>(1, "column1", 123435);
    rows.push_back(std::move(t1));
    t2.set<c1, c2, c3>(2, "column2", 1235);
    rows.push_back(std::move(t2));

    mytable::insert_or_replace_rows(&engine, rows);

    auto ret1 = mytable::select<c1, c2, c3>(&engine).eval();
}