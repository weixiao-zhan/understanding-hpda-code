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
#include <cstring>
#include <vector>
// for enum
namespace ff {
namespace mysql {
class not_in_enum_or_set : public std::exception {
public:
  not_in_enum_or_set(const char *str) : m_data(str) {
    m_data =
        std::string("cannot find ") + m_data + " in mysql.enum or mysql.set";
  }

  virtual ~not_in_enum_or_set() throw() {}
  virtual const char *what() const throw() { return m_data.c_str(); }

protected:
  std::string m_data;
};
namespace internal {
static constexpr const char *__enum_not_found = "enum_not_found";
template <int Index, const char *... ES> struct find_index_helper {};

template <int Index> struct find_index_helper<Index> {
  static int find(const char *value) { return -1; }
  static const char *find(int index) {
    return __enum_not_found;
  }
};
template <int Index, const char *E, const char *... ES>
struct find_index_helper<Index, E, ES...> {
  static int find(const char *value) {
    if (memcmp(value, E, strlen(value)) == 0) {
      return Index;
    } else {
      return find_index_helper<Index + 1, ES...>::find(value);
    }
  }
  static const char *find(int index) {
    if (index == Index) {
      return E;
    } else {
      return find_index_helper<Index + 1, ES...>::find(index);
    }
  }
};

template <const char *... ES> int find_index(const char *value) {
  return find_index_helper<0, ES...>::find(value);
}
template <const char *... ES> const char *find_value(int i) {
  return find_index_helper<0, ES...>::find(i);
}

} // namespace internal
template <const char *... ES> struct enum_m {
public:
  inline enum_m() : m_data(-1){};
  inline enum_m(const char *s) {
    m_data = internal::find_index<ES...>(s);
    if (m_data < 0) {
      throw not_in_enum_or_set(s);
    }
  };

  inline enum_m(const ::sql::SQLString &s) {
    m_data = internal::find_index<ES...>(s.c_str());
  }
  inline enum_m(const enum_m<ES...> &s) : m_data(s.m_data) {}
  enum_m(enum_m &&s) : m_data(std::move(s.m_data)) {}
  enum_m &operator=(const enum_m &s) {
    if (&s == this) {
      return *this;
    }
    m_data = s.m_data;
    return *this;
  }
  bool operator==(const enum_m<ES...> &e) const { return e.m_data == m_data; }
  bool operator!=(const enum_m<ES...> &e) const { return e.m_data != m_data; }

  const char *value() const { return internal::find_value<ES...>(m_data); }
  int index_value() const { return m_data; }

protected:
  int m_data;
};

template <const char *... ES>
std::ostream &operator<<(std::ostream &out, const enum_m<ES...> &data) {
  out << data.value();
  return out;
}
} // namespace mysql

namespace sql {
namespace internal {
template <const char *... ES> struct dump_helper {
  static void dump(std::stringstream &ss) {}
};
template <const char *E> struct dump_helper<E> {
  static void dump(std::stringstream &ss) { ss << "'" << E << "'"; }
};
template <const char *E, const char *... ES> struct dump_helper<E, ES...> {
  static void dump(std::stringstream &ss) {
    ss << "'" << E << "'"
       << ",";
    dump_helper<ES...>::dump(ss);
  }
};
template <typename T> struct dump_col_type_creation;
template <const char *... ES>
struct dump_col_type_creation<::ff::mysql::enum_m<ES...>> {
  static void dump(std::stringstream &ss) {
    ss << " ENUM (";
    dump_helper<ES...>::dump(ss);
    ss << " )";
  }
};

} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;

template <class STMT, const char *... ES>
struct mysql_bind_setter<STMT, ::ff::mysql::enum_m<ES...>> {
  static void bind(STMT stmt, int index,
                   const ::ff::mysql::enum_m<ES...> &value) {
    stmt->setString(index, value.value());
  }
};

template <class T> struct mysql_rs_getter;
template <const char *... ES>
struct mysql_rs_getter<::ff::mysql::enum_m<ES...>> {
  template <typename RST>
  static ::ff::mysql::enum_m<ES...> get(RST r, const std::string &name) {
    return ::ff::mysql::enum_m<ES...>(r->getString(name));
  }
};
} // namespace sql
} // namespace ff

// for set
namespace ff {
namespace mysql {
template <const char *... ES> struct set_m {
public:
  inline set_m() : m_data(){};
  inline set_m(const char *s) {
    auto i = internal::find_index<ES...>(s);
    if (i >= 0) {
      m_data.push_back(i);
    } else {
      throw not_in_enum_or_set(s);
    }
  };

  inline set_m(const ::sql::SQLString &s) {
    auto i = internal::find_index<ES...>(s.c_str());
    if (i >= 0) {
      m_data.push_back(i);
    } else {
      throw not_in_enum_or_set(s.c_str());
    }
  }
  inline set_m(const set_m<ES...> &s) : m_data(s.m_data) {}

  set_m(const std::vector<std::string> &s) {
    for (auto str : s) {
      auto i = internal::find_index<ES...>(str.c_str());
      if (i >= 0) {
        m_data.push_back(i);
      } else {
        throw not_in_enum_or_set(str.c_str());
      }
    }
  }

  bool operator==(const enum_m<ES...> &e) const {
    if (m_data.size() != e.m_data.size()) {
      return false;
    }
    for (uint32_t i = 0; i < m_data.size(); i++) {
      if (m_data[i] != e.m_data[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const enum_m<ES...> &e) const { return !operator==(e); }

  std::vector<std::string> value() const {
    std::vector<std::string> ret;
    for (auto i : m_data) {
      auto c = internal::find_value<ES...>(i);
      if (c) {
        ret.push_back(std::string(c));
      }
    }
    return ret;
  }
  std::vector<int> index_value() const { return m_data; }

  // TODO we may have some iterators here

protected:
  std::vector<int> m_data;
};
} // namespace mysql

namespace sql {
namespace internal {
template <typename T> struct dump_col_type_creation;
template <const char *... ES>
struct dump_col_type_creation<::ff::mysql::set_m<ES...>> {
  static void dump(std::stringstream &ss) {
    ss << " SET (";
    dump_helper<ES...>::dump(ss);
    ss << " )";
  }
};

} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;
template <class STMT, const char *... ES>
struct mysql_bind_setter<STMT, ::ff::mysql::set_m<ES...>> {
  static void bind(STMT stmt, int index,
                   const ::ff::mysql::set_m<ES...> &value) {
    std::stringstream ss;
    auto vs = value.value();
    for (uint32_t i = 0; i < vs.size(); ++i) {
      ss << vs[i];
      if (i + 1 < vs.size()) {
        ss << ",";
      }
    }
    stmt->setString(index, ss.str());
  }
};

template <class T> struct mysql_rs_getter;
template <const char *... ES>
struct mysql_rs_getter<::ff::mysql::set_m<ES...>> {
  template <typename RST>
  static ::ff::mysql::set_m<ES...> get(RST r, const std::string &name) {
    std::string data(r->getString(name));

    std::vector<std::string> vs;
    // split
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = data.find(',', pos)) != std::string::npos) {
      std::string substring(data.substr(prev_pos, pos - prev_pos));
      vs.push_back(substring);
      prev_pos = ++pos;
    }
    vs.push_back(data.substr(prev_pos, pos - prev_pos)); // Last word
    // split done

    return ::ff::mysql::set_m<ES...>(vs);
  }
};

} // namespace sql
} // namespace ff
