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
#include <cppconn/sqlstring.h>

namespace ff {
namespace mysql {
struct medium_int {
public:
  inline medium_int() : m_data(){};
  inline medium_int(int value) : m_data(value){};

  inline const int32_t &data() const { return m_data; }
  inline int32_t &data() { return m_data; }

protected:
  int32_t m_data;
};

struct medium_uint {
public:
  inline medium_uint() : m_data(){};
  inline medium_uint(uint32_t value) : m_data(value){};

  inline const uint32_t &data() const { return m_data; }
  inline uint32_t &data() { return m_data; }

protected:
  uint32_t m_data;
};

} // namespace mysql
namespace sql {
namespace internal {

template <typename T> struct dump_col_type_creation;
template <> struct dump_col_type_creation<::ff::mysql::medium_int> {
  static void dump(std::stringstream &ss) { ss << " MEDIUMINT "; }
};
template <> struct dump_col_type_creation<::ff::mysql::medium_uint> {
  static void dump(std::stringstream &ss) { ss << " MEDIUMINT UNSIGNED "; }
};

} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;

template <class STMT> struct mysql_bind_setter<STMT, ::ff::mysql::medium_int> {
  static void bind(STMT stmt, int index, const ::ff::mysql::medium_int &value) {
    stmt->setInt(index, value.data());
  }
};

template <class STMT> struct mysql_bind_setter<STMT, ::ff::mysql::medium_uint> {
  static void bind(STMT stmt, int index,
                   const ::ff::mysql::medium_uint &value) {
    stmt->setUInt(index, value.data());
  }
};

template <class T> struct mysql_rs_getter;
template <> struct mysql_rs_getter<::ff::mysql::medium_int> {
  template <typename RST>
  static ::ff::mysql::medium_int get(RST r, const std::string &name) {
    return ::ff::mysql::medium_int(r->getInt(name));
  }
};

template <> struct mysql_rs_getter<::ff::mysql::medium_uint> {
  template <typename RST>
  static ::ff::mysql::medium_uint get(RST r, const std::string &name) {
    return ::ff::mysql::medium_uint(r->getUInt(name));
  }
};

} // namespace sql
} // namespace ff
