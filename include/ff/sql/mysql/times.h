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
#include <chrono>
#include <cppconn/sqlstring.h>
#include <iomanip>
/**
types method of date, time, datetime, timestamp, year are defined in setDatetime and getString,
the type defination as one data structure MYSQL_TIME on : usr/include/mysql/mysql_time.h
It seem that the difference of all time type will be separated in the setDatetime and getString.
The important is how to use these type like time, date in the fflib.
*/

// for timestamp
namespace ff {
namespace sql {
namespace internal {
template <std::intmax_t Denom> struct log_value {};
template <> struct log_value<1> { constexpr static int value = 0; };
template <> struct log_value<10> { constexpr static int value = 1; };
template <> struct log_value<100> { constexpr static int value = 2; };
template <> struct log_value<1000> { constexpr static int value = 3; };
template <> struct log_value<10000> { constexpr static int value = 4; };
template <> struct log_value<100000> { constexpr static int value = 5; };
template <> struct log_value<1000000> { constexpr static int value = 6; };
template <> struct log_value<10000000> { constexpr static int value = 7; };
template <> struct log_value<100000000> { constexpr static int value = 8; };
template <> struct log_value<1000000000> { constexpr static int value = 9; };
template <> struct log_value<10000000000> { constexpr static int value = 10; };

template <class T> struct dump_col_type_creation;
template <typename Rep, std::intmax_t Denom>
struct dump_col_type_creation<
    std::chrono::time_point<std::chrono::steady_clock,
                            std::chrono::duration<Rep, std::ratio<1, Denom>>>> {
  static void dump(std::stringstream &ss) {
    static_assert(log_value<Denom>::value <= 6,
                  "according to MYSQL, *fsp* of timestamp support up to 6, "
                  "i.e., microsecods");
    if (log_value<Denom>::value != 0) {
      ss << " TIMESTAMP (" << log_value<Denom>::value
         << ") DEFAULT CURRENT_TIMESTAMP(" << log_value<Denom>::value << ") ";
    } else {
      ss << " TIMESTAMP DEFAULT CURRENT_TIMESTAMP";
    }
  }
};

template <typename Rep, std::intmax_t Denom>
struct dump_col_type_creation<
    std::chrono::time_point<std::chrono::system_clock,
                            std::chrono::duration<Rep, std::ratio<1, Denom>>>> {
  static void dump(std::stringstream &ss) {
    static_assert(log_value<Denom>::value <= 6,
                  "according to MYSQL, *fsp* of timestamp support up to 6, "
                  "i.e., microsecods");
    if (log_value<Denom>::value != 0) {
      ss << " DATETIME (" << log_value<Denom>::value
         << ") DEFAULT CURRENT_TIMESTAMP(" << log_value<Denom>::value << ") ";
    } else {
      ss << " DATETIME DEFAULT CURRENT_TIMESTAMP";
    }
  }
};

} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;
template <class STMT, class Clock, class Duration>
struct mysql_bind_setter<STMT, std::chrono::time_point<Clock, Duration>> {
  static void bind(STMT stmt, int index,
                   const std::chrono::time_point<Clock, Duration> &value) {
    std::stringstream ss;
    const std::time_t t_c = std::chrono::system_clock::to_time_t(value);
    typedef typename Duration::period period_t;
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%d %T") << "."
       << value.time_since_epoch().count() % period_t::den;
    stmt->setDateTime(index, ss.str());
  }
};

template <class T> struct mysql_rs_getter;
template <class Clock, class Duration>
struct mysql_rs_getter<std::chrono::time_point<Clock, Duration>> {
  template <typename RST>
  static std::chrono::time_point<Clock, Duration> get(RST r,
                                                      const std::string &name) {
    std::istringstream ss(r->getString(name));
    std::tm tm;
    ss >> std::get_time(&tm, "%Y-%m-%d %T");
    char ks = '.';
    int32_t v = 0;
    ss >> ks;
    ss >> v;
    auto from = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::time_point_cast<Duration>(from + Duration(v));
  }
};

} // namespace sql
} // namespace ff

// for date
namespace ff {
namespace mysql {
typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::hours>
    date;
} // namespace mysql

namespace sql {
namespace internal {

template <> struct dump_col_type_creation<::ff::mysql::date> {
  static void dump(std::stringstream &ss) { ss << " DATE "; }
};

} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;
template <class STMT> struct mysql_bind_setter<STMT, ::ff::mysql::date> {
  static void bind(STMT stmt, int index, const ::ff::mysql::date &value) {
    std::stringstream ss;
    const std::time_t t_c = std::chrono::system_clock::to_time_t(value);
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%d");
    stmt->setDateTime(index, ss.str());
  }
};

template <class T> struct mysql_rs_getter;
template <> struct mysql_rs_getter<::ff::mysql::date> {
  template <typename RST>
  static ::ff::mysql::date get(RST r, const std::string &name) {
    std::istringstream ss(r->getString(name));
    std::tm tm;
    ss >> std::get_time(&tm, "%Y-%m-%d");
    auto from = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return std::chrono::time_point_cast<::ff::mysql::date::duration>(from);
  }
};

} // namespace sql
} // namespace ff

// for year
namespace ff {
namespace mysql {
struct year {
public:
  inline year() : m_data(){};
  inline year(int value) : m_data(value){};

  inline const int32_t &data() const { return m_data; }
  inline int32_t &data() { return m_data; }

protected:
  int32_t m_data;
};
} // namespace mysql

namespace sql {
namespace internal {

template <> struct dump_col_type_creation<::ff::mysql::year> {
  static void dump(std::stringstream &ss) { ss << " YEAR "; }
};

} // namespace internal
template <class STMT, class T> struct mysql_bind_setter;
template <class STMT> struct mysql_bind_setter<STMT, ::ff::mysql::year> {
  static void bind(STMT stmt, int index, const ::ff::mysql::year &value) {
    std::stringstream ss;
    ss << value.data();
    stmt->setDateTime(index, ss.str());
  }
};

template <class T> struct mysql_rs_getter;
template <> struct mysql_rs_getter<::ff::mysql::year> {
  template <typename RST>
  static ::ff::mysql::year get(RST r, const std::string &name) {
    std::istringstream ss(r->getString(name));
    ::ff::mysql::year y;
    ss >> y.data();
    return y;
  }
};

} // namespace sql
} // namespace ff

// for time
namespace ff {

namespace sql {
namespace internal {
template <class Rep, class Period>
struct dump_col_type_creation<std::chrono::duration<Rep, Period>> {
  static void dump(std::stringstream &ss) { ss << " TIME "; }
};

template <class Rep, std::intmax_t Denom>
struct dump_col_type_creation<
    std::chrono::duration<Rep, std::ratio<1, Denom>>> {
  static void dump(std::stringstream &ss) {
    if (log_value<Denom>::value != 0) {
      ss << " TIME (" << log_value<Denom>::value << ") ";
    } else {
      ss << " TIME ";
    }
  }
};
} // namespace internal

template <class STMT, class T> struct mysql_bind_setter;
template <class STMT, class Rep, class Period>
struct mysql_bind_setter<STMT, std::chrono::duration<Rep, Period>> {
  static void bind(STMT stmt, int index,
                   const std::chrono::duration<Rep, Period> &value) {
    std::stringstream ss;
    auto h = std::chrono::duration_cast<std::chrono::hours>(value).count();
    auto m = std::chrono::duration_cast<std::chrono::minutes>(value).count() -
             h * 60;
    auto s = std::chrono::duration_cast<std::chrono::seconds>(value).count() -
             h * 60 * 60 - m * 60;
    if (h < 100) {
      ss << std::internal << std::setfill('0') << std::setw(2) << h << ":";
    } else {
      ss << std::internal << std::setfill('0') << std::setw(3) << h << ":";
    }
    ss << std::setfill('0') << std::setw(2) << m << ":";
    ss << std::setfill('0') << std::setw(2) << s;

    if (Period::den > 1) {
      ss << "." << value.count() % Period::den;
    }
    stmt->setDateTime(index, ss.str());
  }
};

template <class T> struct mysql_rs_getter;
template <class Rep, class Period>
struct mysql_rs_getter<std::chrono::duration<Rep, Period>> {
  template <typename RST>
  static std::chrono::duration<Rep, Period> get(RST r,
                                                const std::string &name) {
    std::istringstream ss(r->getString(name));
    int32_t h = 0;
    int32_t m = 0;
    int32_t s = 0;
    int32_t f = 0;
    char c = '.';
    ss >> h >> c >> m >> c >> s >> c >> f;
    std::chrono::duration<Rep, Period> ret =
        std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(
            std::chrono::hours(h) + std::chrono::minutes(m) +
            std::chrono::seconds(s) + std::chrono::duration<Rep, Period>(f));

    return ret;
  }
};

} // namespace sql
} // namespace ff
