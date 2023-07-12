/***********************************************
  The MIT License (MIT)

  Copyright (c) 2022 zi wang <zzziwang@outlook.com>

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
#pragma once
#ifndef FF_SUPPORT_SQL_DECIMAL
#error "Have to define FF_SUPPORT_SQL_DECIMAL to include this file"
#endif

#include "ff/vendor/decimal.h"
#include "ff/sql/table_create.h"
#include <cppconn/sqlstring.h>



//for decimal
namespace ff {
namespace mysql {

template <int P, int D> class decimal : public ::dec::decimal<D> {
  static_assert(P <= 65 && P >= 1, "P should be between 1 to 65.");
  static_assert(D >= 0 && D <= 30, "D should be between 0 to 30.");
  static_assert(P >= D, "P must be >= D");

public:
  typedef ::dec::decimal<D> base;
  decimal() : base(0){};
  template <typename T> decimal(const T &t) : base(t) {}
  decimal(const char *s) : base(std::string(s)){};
  decimal(const ::sql::SQLString &s) : base(std::string(s.c_str())) {}
};

} // namespace mysql
namespace sql {
namespace internal {

template <typename T> struct dump_col_type_creation;
template <int P, int D>
struct dump_col_type_creation<::ff::mysql::decimal<P, D>> {
  static void dump(std::stringstream &ss) {
    ss << " Decimal(" << P << ", " << D << ") ";
  }
};
} // namespace internal

template <class STMT, class T> struct mysql_bind_setter;

template <class STMT, int P, int D>
struct mysql_bind_setter<STMT, ::ff::mysql::decimal<P, D>> {
  static void bind(STMT stmt, int index,
                   const ::ff::mysql::decimal<P, D> &value) {
    stmt->setString(index, ::dec::toString(value));
  }
};

template <class T> struct mysql_rs_getter;
template <int P, int D> struct mysql_rs_getter<::ff::mysql::decimal<P, D>> {
  template <typename RST>
  static ::ff::mysql::decimal<P, D> get(RST r, const std::string &name) {
    return ::ff::mysql::decimal<P, D>(r->getString(name));
  }
};

} // namespace sql
} // namespace ff

