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
#include "ff/sql/table_create.h"
#include <assert.h>
#include <bitset>
#include <cppconn/sqlstring.h>

#define bitMax 64


// for BIT
namespace ff {
namespace sql {
namespace internal {
template <typename T> struct dump_col_type_creation;
template <std::size_t Len> struct dump_col_type_creation<std::bitset<Len>> {
  static void dump(std::stringstream &ss) { ss << " BIT(" << Len << ") "; }
};
} // namespace internal

template <class STMT, class T> struct mysql_bind_setter;

template <class STMT, std::size_t Len>
struct mysql_bind_setter<STMT, std::bitset<Len>> {
  static void bind(STMT stmt, int index, const std::bitset<Len> &value) {
    stmt->setUInt64(index, value.to_ullong());
  }
};

template <class T> struct mysql_rs_getter;
template <std::size_t Len> struct mysql_rs_getter<std::bitset<Len>> {
  template <typename RST>
  static std::bitset<Len> get(RST r, const std::string &name) {
    return std::bitset<Len>(r->getUInt64(name));
  }
};

} // namespace sql
} // namespace ff




// for binary
namespace ff {
namespace mysql {
template <uint32_t Len> struct binary : public std::string {
public:
  using std::string::string;
  binary(const ::sql::SQLString &s) : std::string(s.asStdString()) {}
};

} // namespace mysql
namespace sql {
namespace internal {
template <class T> struct dump_col_type_creation;
template <uint32_t Len>
struct dump_col_type_creation<::ff::mysql::binary<Len>> {
  static void dump(std::stringstream &ss) { ss << " BINARY(" << Len << ") "; }
};
} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;
template <class STMT, uint32_t Len>
struct mysql_bind_setter<STMT, ::ff::mysql::binary<Len>> {
  static void bind(STMT stmt, int index,
                   const ::ff::mysql::binary<Len> &value) {
    stmt->setString(index, value);
  }
};

template <class T> struct mysql_rs_getter;
template <uint32_t Len> struct mysql_rs_getter<::ff::mysql::binary<Len>> {
  template <typename RST>
  static ::ff::mysql::binary<Len> get(RST r, const std::string &name) {
    return ::ff::mysql::binary<Len>(r->getString(name));
  }
};

} // namespace sql
} // namespace ff




// for varbin
namespace ff {
namespace mysql {
template <uint32_t Len> struct var_binary : public std::string {
public:
  using std::string::string;
  var_binary(const ::sql::SQLString &s) : std::string(s.asStdString()) {}
};

} // namespace mysql
namespace sql {
namespace internal {
template <class T> struct dump_col_type_creation;
template <uint32_t Len>
struct dump_col_type_creation<::ff::mysql::var_binary<Len>> {
  static void dump(std::stringstream &ss) {
    ss << " VARBINARY(" << Len << ") ";
  }
};
} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;
template <class STMT, uint32_t Len>
struct mysql_bind_setter<STMT, ::ff::mysql::var_binary<Len>> {
  static void bind(STMT stmt, int index,
                   const ::ff::mysql::var_binary<Len> &value) {
    stmt->setString(index, value);
  }
};

template <class T> struct mysql_rs_getter;
template <uint32_t Len> struct mysql_rs_getter<::ff::mysql::var_binary<Len>> {
  template <typename RST>
  static ::ff::mysql::var_binary<Len> get(RST r, const std::string &name) {
    return ::ff::mysql::var_binary<Len>(r->getString(name));
  }
};

} // namespace sql
} // namespace ff
