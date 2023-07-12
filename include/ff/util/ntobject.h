/***********************************************
  The MIT License (MIT)

  Copyright (c) 2012 Athrun Arthur <athrunarthur@gmail.com>

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
#include "ff/util/internal/user_new_type.h"
#include "ff/util/preprocessor.h"
#include "ff/util/tuple_type.h"
#include "ff/util/type_list.h"
#include <memory>
#include <vector>

namespace ff {
namespace util {

/*
 * ntobject is used to define a struct without writng your own class.
 * To use it, you need to declare a bunch of types first, like this
 *
 * define_nt(email, std::string, "email");
 * define_nt(uid, uint64_t, "uid");
 * define_nt(_name, std::string, "name");
 *
 * typedef ntobject<email, uid, name> myobject_t;
 *
 * Now you have your own structure -- myobject_t, you can set and get field in
 * it like
 *
 * myobject_t obj;
 * obj.set<_name>("xuepeng");
 * obj.set<email>("xp@example.com");
 * obj.set<uid>(128);
 *
 * std::string val_name = obj.get<name>();
 *
 * Further more, you can set multiple values with any order if you want,
 *
 * obj.set<name, email>("xuepeng", "xp@example.com");
 * obj.set<email, uid, name> ("xp@example.com", 128, "xuepeng");
 *
 *
 * */
template <typename... ARGS> class ntobject {
public:
  typedef typename util::type_list<ARGS...> type_list;
  typedef
      typename convert_type_list_to_tuple<typename nt_extract_content_type_list<
          util::type_list<ARGS...>>::type>::type content_type;

  ntobject() : m_content(new content_type()) {}
  template <typename OT>
  ntobject(const OT &data) : m_content(new content_type()) {
    assign_helper<OT, ARGS...>(data);
  }

  ntobject<ARGS...> make_copy() const {
    ntobject<ARGS...> rt;
    *rt.m_content = *m_content;
    return rt;
  }

  template <typename CT>
  void set(const typename internal::nt_traits<CT>::type &val) {
    static_assert(is_type_in_type_list<CT, util::type_list<ARGS...>>::value,
                  "Cannot set a value that's not in the ntobject/row!");
    const static int index =
        get_index_of_type_in_typelist<CT, util::type_list<ARGS...>>::value;
    std::get<index>(*m_content) = val;
  }

  template <typename CT, typename CT1, typename... CARGS, typename... PARGS>
  void set(const typename internal::nt_traits<CT>::type &val,
           const typename internal::nt_traits<CT1>::type &val1,
           PARGS... params) {
    static_assert(is_type_in_type_list<CT, util::type_list<ARGS...>>::value,
                  "Cannot set a value that's not in the row!");
    static_assert(is_type_in_type_list<CT1, util::type_list<ARGS...>>::value,
                  "Cannot set a value that's not in the row!");
    const static int index =
        get_index_of_type_in_typelist<CT, util::type_list<ARGS...>>::value;
    std::get<index>(*m_content) = val;

    set<CT1, CARGS...>(val1, params...);
  }

  template <typename CT> typename internal::nt_traits<CT>::type get() const {
    static_assert(is_type_in_type_list<CT, util::type_list<ARGS...>>::value,
                  "Cannot get a value that's not in the ntobject/row!");
    const static int index =
        get_index_of_type_in_typelist<CT, util::type_list<ARGS...>>::value;
    return std::get<index>(*m_content);
  }

  ntobject<ARGS...> &operator=(const ntobject<ARGS...> &data) {
    if ((void *)&data == (void *)this) {
      return *this;
    }
    assign_helper<ntobject<ARGS...>, ARGS...>(data);
    return *this;
  }

  template <typename OT> ntobject<ARGS...> &operator=(const OT &data) {
    if ((void *)&data == (void *)this) {
      return *this;
    }
    assign_helper<OT, ARGS...>(data);
    return *this;
  }

protected:
  template <typename OT, typename CT>
  auto assign_helper(const OT &data) -> typename std::enable_if<
      is_type_in_type_list<CT, typename OT::type_list>::value, void>::type {
    set<CT>(data.template get<CT>());
  }
  template <typename OT, typename CT>
  auto assign_helper(const OT &data) -> typename std::enable_if<
      !is_type_in_type_list<CT, typename OT::type_list>::value, void>::type {}

  template <typename OT, typename CT, typename CT1, typename... CARGS>
  auto assign_helper(const OT &data) -> typename std::enable_if<
      is_type_in_type_list<CT, typename OT::type_list>::value, void>::type {
    set<CT>(data.template get<CT>());
    assign_helper<OT, CT1, CARGS...>(data);
  }

  template <typename OT, typename CT, typename CT1, typename... CARGS>
  auto assign_helper(const OT &data) -> typename std::enable_if<
      !is_type_in_type_list<CT, typename OT::type_list>::value, void>::type {
    assign_helper<OT, CT1, CARGS...>(data);
  }

protected:
  std::shared_ptr<content_type> m_content;
};

template <typename... ARGS> class ntarray {
public:
  typedef ntobject<ARGS...> row_type;

  void push_back(const row_type &row) { m_collection.push_back(row); }

  void push_back(row_type &&row) { m_collection.push_back(std::move(row)); }

  void clear() { m_collection.clear(); }

  size_t size() const { return m_collection.size(); }

  bool empty() const { return m_collection.empty(); }

  row_type &operator[](size_t index) { return m_collection[index]; }

  const row_type &operator[](size_t index) const { return m_collection[index]; }

protected:
  std::vector<row_type> m_collection;
};

template <typename T> struct is_ntobject { const static bool value = false; };

template <typename... ARGS> struct is_ntobject<ntobject<ARGS...>> {
  const static bool value = true;
};

template <typename T> struct is_ntarray { const static bool value = false; };

template <typename... ARGS> struct is_ntarray<ntarray<ARGS...>> {
  const static bool value = true;
};

template <typename T> struct type_of_nt {
  typedef typename internal::nt_traits<T>::type type;
};

template <typename T> struct name_of_nt {
  constexpr static const char *name = internal::nt_traits<T>::name;
};

template <typename T, typename MT> struct append_type {
  static_assert(sizeof(T) == -1, "you can only append type to ntobject");
};

template <typename MT, typename... ARGS>
struct append_type<ntobject<ARGS...>, MT> {
  typedef ntobject<ARGS..., MT> type;

  static type value(const ntobject<ARGS...> &n,
                    const typename type_of_nt<MT>::type &m) {
    type t;
    copy_helper<0>::copy(t, n);
    t.template set<MT>(m);
    return t;
  }

  static type value(ntobject<ARGS...> &&n,
                    const typename type_of_nt<MT>::type &m) {
    type t;
    copy_helper<0>::copy(t, std::move(n));
    t.template set<MT>(m);
    return t;
  }

private:
  template <int Index> struct copy_helper {
    // TODO optimize for rvalue(&&)
    template <typename T1, typename T2>
    static auto copy(T1 &target, T2 &&source) -> typename std::enable_if<
        (std::remove_reference<T2>::type::type_list::len > Index), void>::type {
      typedef typename get_type_at_index_in_typelist<
          typename std::remove_reference<T2>::type::type_list, Index>::type
          c_type;
      target.template set<c_type>(source.template get<c_type>());
      copy_helper<Index + 1>::copy(target, std::forward<T2>(source));
    }

    template <typename T1, typename T2>
    static auto copy(T1 &target, const T2 &source) -> typename std::enable_if<
        (std::remove_reference<T2>::type::type_list::len <= Index),
        void>::type {}
  };
};

} // namespace util
} // namespace ff

