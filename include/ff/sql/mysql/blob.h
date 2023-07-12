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
#include <cppconn/sqlstring.h>
#include <fstream>
#include <iostream>

#define bitMax 64

// for Blob(using string method)
namespace ff {
namespace mysql {
struct tiny_blob_flag {};
struct blob_flag {};
struct medium_blob_flag {};
struct long_blob_flag {};

template <typename FT> struct blob_m {
public:
  inline blob_m() : ms_data(nullptr) {}
  inline blob_m(std::istream *s) : ms_data(s){};

  inline std::istream *stream() const { return ms_data; }
  inline std::istream *&stream() { return ms_data; }

protected:
  std::istream *ms_data;
};

using tiny_blob = blob_m<tiny_blob_flag>;
using blob = blob_m<blob_flag>;
using medium_blob = blob_m<medium_blob_flag>;
using long_blob = blob_m<long_blob_flag>;

} // namespace mysql
namespace sql {
namespace internal {
template <class T> struct dump_col_type_creation;
template <>
struct dump_col_type_creation<
    ::ff::mysql::blob_m<::ff::mysql::tiny_blob_flag>> {
  static void dump(std::stringstream &ss) { ss << " TINYBLOB "; }
};
template <>
struct dump_col_type_creation<::ff::mysql::blob_m<::ff::mysql::blob_flag>> {
  static void dump(std::stringstream &ss) { ss << " BLOB "; }
};
template <>
struct dump_col_type_creation<
    ::ff::mysql::blob_m<::ff::mysql::medium_blob_flag>> {
  static void dump(std::stringstream &ss) { ss << " MEDIUMBLOB "; }
};
template <>
struct dump_col_type_creation<
    ::ff::mysql::blob_m<::ff::mysql::long_blob_flag>> {
  static void dump(std::stringstream &ss) { ss << " LONGBLOB "; }
};
} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;

template <class STMT, class FT>
struct mysql_bind_setter<STMT, ::ff::mysql::blob_m<FT>> {
  static void bind(STMT stmt, int index, const ::ff::mysql::blob_m<FT> &value) {
    stmt->setBlob(index, value.stream());
  }
};

template <class T> struct mysql_rs_getter;
template <class FT> struct mysql_rs_getter<::ff::mysql::blob_m<FT>> {
  template <typename RST>
  static ::ff::mysql::blob_m<FT> get(RST r, const std::string &name) {
    auto is = r->getBlob(name);
    return ::ff::mysql::blob_m<FT>(is);
  }
};

} // namespace sql
} // namespace ff

