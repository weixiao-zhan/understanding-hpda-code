/***********************************************
  The MIT License (MIT)

  Copyright (c) 2012-2020 Athrun Arthur <athrunarthur@gmail.com>

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

//! TODO not implemented methods in ntcompact_array
// insert
// erase
// rbegin, rend riterator
//
// Add column check for push_back with template
#pragma once
#include "ff/util/internal/user_new_type.h"
#include "ff/util/preprocessor.h"
#include "ff/util/tuple_type.h"
#include "ff/util/type_list.h"
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

namespace ff {
namespace util {
template <typename... ARGS> class ntobject;

template <typename... ARGS> struct nt_collect {
  typedef typename util::type_list<ARGS...> type_list;
};

template <typename... ARGS> struct nt_layout {
  typedef typename util::type_list<ARGS...> type_list;
};

namespace internal {

template <typename... ARGS> class nt_collect_storage;

template <typename ARG, typename... ARGS>
class nt_collect_storage<ARG, ARGS...> {
public:
  typedef typename util::type_list<ARG, ARGS...> type_list;
  typedef std::vector<
      typename convert_type_list_to_tuple<typename nt_extract_content_type_list<
          util::type_list<ARG, ARGS...>>::type>::type>
      data_type;

  template <typename CT>
  void set(size_t index, const typename internal::nt_traits<CT>::type &val) {
    static_assert(
        is_type_in_type_list<CT, util::type_list<ARG, ARGS...>>::value,
        "Cannot set a value that's not in nt_collect_storage!");
    const static int Index =
        get_index_of_type_in_typelist<CT, util::type_list<ARG, ARGS...>>::value;
    std::get<Index>(m_data[index]) = val;
  }

  template <typename CT, typename CT1, typename... CARGS, typename... PARGS>
  void set(size_t index, const typename internal::nt_traits<CT>::type &val,
           const typename internal::nt_traits<CT1>::type &val1,
           PARGS... params) {
    static_assert(
        is_type_in_type_list<CT, util::type_list<ARG, ARGS...>>::value,
        "Cannot set a value that's not in nt_collect_storage!");
    static_assert(
        is_type_in_type_list<CT1, util::type_list<ARG, ARGS...>>::value,
        "Cannot set a value that's not in nt_collect_storage!");
    const static int Index =
        get_index_of_type_in_typelist<CT, util::type_list<ARG, ARGS...>>::value;
    std::get<Index>(m_data[index]) = val;

    set<CT1, CARGS...>(index, val1, params...);
  }

  template <typename CT>
  typename internal::nt_traits<CT>::type get(size_t index) const {
    static_assert(
        is_type_in_type_list<CT, util::type_list<ARG, ARGS...>>::value,
        "Cannot get a value that's not in nt_collect_storage!");
    const static int Index =
        get_index_of_type_in_typelist<CT, util::type_list<ARG, ARGS...>>::value;
    return std::get<Index>(m_data[index]);
  }

  size_t size() const { return m_data.size(); }
  bool empty() const { return m_data.empty(); }
  void emplace_back() { m_data.emplace_back(); }
  void clear() { m_data.clear(); }
  void pop_back() { m_data.pop_back(); }

  template <typename PType> void set_from_obj(size_t index, const PType &v) {
    set_from_obj_helper<0>(index, v);
  }
  data_type &data() { return m_data; }
  const data_type &data() const { return m_data; }

protected:
  template <int Index, typename PType>
  auto set_from_obj_helper(size_t index, const PType &val) ->
      typename std::enable_if<(Index >= type_list::len), void>::type {}

  template <int Index, typename PType>
  auto set_from_obj_helper(size_t index, const PType &val) ->
      typename std::enable_if<(Index < type_list::len), void>::type {
    using target_type =
        typename get_type_at_index_in_typelist<type_list, Index>::type;
    std::get<Index>(m_data[index]) = val.template get<target_type>();
    set_from_obj_helper<Index + 1>(index, val);
  }

protected:
  typedef
      typename convert_type_list_to_tuple<typename nt_extract_content_type_list<
          util::type_list<ARG, ARGS...>>::type>::type content_type;
  std::vector<content_type> m_data;
};

template <typename ARG> class nt_collect_storage<ARG> {
public:
  typedef typename util::type_list<ARG> type_list;
  typedef std::vector<typename nt_traits<ARG>::type> data_type;

  template <typename CT>
  void set(size_t index, const typename internal::nt_traits<CT>::type &val) {
    static_assert(is_type_in_type_list<CT, util::type_list<ARG>>::value,
                  "Cannot set a value that's not in nt_collect_storage!");
    m_data[index] = val;
  }
  template <typename CT>
  typename internal::nt_traits<CT>::type get(size_t index) const {
    static_assert(is_type_in_type_list<CT, util::type_list<ARG>>::value,
                  "Cannot get a value that's not in the ntobject/row!");
    return m_data[index];
  }

  size_t size() const { return m_data.size(); }
  bool empty() const { return m_data.empty(); }
  void emplace_back() { m_data.emplace_back(); }
  void clear() { m_data.clear(); }
  void pop_back() { m_data.pop_back(); }
  template <typename PType> void set_from_obj(size_t index, const PType &v) {
    m_data[index] = v.template get<ARG>();
  }

  data_type &data() { return m_data; }
  const data_type &data() const { return m_data; }

protected:
  std::vector<typename nt_traits<ARG>::type> m_data;
};

template <typename CollectType> struct get_collect_storage_type {};
template <typename... ARGS>
struct get_collect_storage_type<nt_collect<ARGS...>> {
  typedef nt_collect_storage<ARGS...> type;
};

template <typename TL> struct map_collect_list_to_storage_list {};
template <typename T1, typename T2, typename... TS>
struct map_collect_list_to_storage_list<type_list<T1, T2, TS...>> {
  typedef typename merge_type_list<
      type_list<typename get_collect_storage_type<T1>::type>,
      typename map_collect_list_to_storage_list<type_list<T2, TS...>>::type>::
      type type;
};
template <typename T1> struct map_collect_list_to_storage_list<type_list<T1>> {
  typedef type_list<typename get_collect_storage_type<T1>::type> type;
};

template <typename TC, typename CollectTypeList>
struct get_index_of_type_in_collect_typelist {
  const static int value = -1;
};
template <typename TC, typename TL1, typename... TS>
struct get_index_of_type_in_collect_typelist<TC, type_list<TL1, TS...>> {
  const static int value = std::conditional<
      is_type_in_type_list<TC, typename TL1::type_list>::value,
      int_number_type<0>,
      int_number_type<1 + get_index_of_type_in_collect_typelist<
                              TC, type_list<TS...>>::value>>::type::value;
};

template <typename... ARGS> class nt_layout_storage {
public:
  template <typename CT>
  void set(size_t index, const typename internal::nt_traits<CT>::type &val) {
    const static int CIndex =
        get_index_of_type_in_collect_typelist<CT,
                                              util::type_list<ARGS...>>::value;
    static_assert(CIndex != -1,
                  "Cannot set a value that's not in the nt_object!");
    std::get<CIndex>(m_data).template set<CT>(index, val);
  }

  template <typename CT, typename CT1, typename... CARGS, typename... PARGS>
  void set(size_t index, const typename internal::nt_traits<CT>::type &val,
           const typename internal::nt_traits<CT1>::type &val1,
           PARGS... params) {
    const static int CIndex =
        get_index_of_type_in_collect_typelist<CT,
                                              util::type_list<ARGS...>>::value;
    static_assert(CIndex != -1,
                  "Cannot set a value that's not in the nt_object!");
    std::get<CIndex>(m_data).template set<CT>(index, val);

    set<CT1, CARGS...>(index, val1, params...);
  }

  template <typename CT>
  typename internal::nt_traits<CT>::type get(size_t index) const {
    const static int CIndex =
        get_index_of_type_in_collect_typelist<CT,
                                              util::type_list<ARGS...>>::value;
    static_assert(CIndex != -1,
                  "Cannot get a value that's not in the nt_object!");

    // std::cout << "CIndex " << CIndex << std::endl;
    // typename internal::nt_traits<CT>::type t;
    // return t;
    return std::get<CIndex>(m_data).template get<CT>(index);
  }

  template <typename... CARGS, typename... PARGS>
  void push_back_with_elements(PARGS... params) {
    size_t pos = emplace_back<0>();
    set<CARGS...>(pos, params...);
  }

  template <typename PType> void push_back_with_ntobject(const PType &val) {
    size_t pos = emplace_back<0>();
    push_back_set_obj_helper<0>(pos, val);
  }

  bool empty() const { return std::get<0>(m_data).empty(); }

  size_t size() const {
    return std::get<0>(m_data).size();
  }

  void clear() { clear_helper<0>(); }

  void pop_back() { pop_back_helper<0>(); }

  template <typename CollectType>
  auto collect() ->
      typename internal::get_collect_storage_type<CollectType>::type::data_type
          & {
    const static int CIndex =
        get_index_of_type_in_typelist<CollectType,
                                      util::type_list<ARGS...>>::value;
    static_assert(CIndex != -1,
                  "Cannot set a value that's not in the nt_object!");
    return std::get<CIndex>(m_data).data();
  }
  template <typename CollectType>
  auto collect() const -> const
      typename internal::get_collect_storage_type<CollectType>::type::data_type
          & {
    const static int CIndex =
        get_index_of_type_in_typelist<CollectType,
                                      util::type_list<ARGS...>>::value;
    static_assert(CIndex != -1,
                  "Cannot set a value that's not in the nt_object!");
    return std::get<CIndex>(m_data).data();
  }

protected:
  typedef type_list<ARGS...> collect_list;

  template <int Index>
  auto emplace_back() ->
      typename std::enable_if<(Index >= collect_list::len), size_t>::type {
    return 0;
  }

  template <int Index>
  auto emplace_back() ->
      typename std::enable_if<(Index < collect_list::len), size_t>::type {
    std::get<Index>(m_data).emplace_back();
    emplace_back<Index + 1>();
    return std::get<Index>(m_data).size() - 1;
  }

  template <int Index>
  auto clear_helper() ->
      typename std::enable_if<(Index >= collect_list::len), size_t>::type {}

  template <int Index>
  auto clear_helper() ->
      typename std::enable_if<(Index < collect_list::len), size_t>::type {
    std::get<Index>(m_data).clear();
    clear_helper<Index + 1>();
  }
  template <int Index>
  auto pop_back_helper() ->
      typename std::enable_if<(Index >= collect_list::len), size_t>::type {}
  template <int Index>
  auto pop_back_helper() ->
      typename std::enable_if<(Index < collect_list::len), size_t>::type {
    std::get<Index>(m_data).pop_back();
    pop_back_helper<Index + 1>();
  }

  template <int Index, typename PType>
  auto push_back_set_obj_helper(size_t last_index, const PType &v) ->
      typename std::enable_if<(Index >= collect_list::len), void>::type {}

  template <int Index, typename PType>
  auto push_back_set_obj_helper(size_t last_index, const PType &v) ->
      typename std::enable_if<(Index < collect_list::len), void>::type {
    std::get<Index>(m_data).set_from_obj(last_index, v);
    push_back_set_obj_helper<Index + 1>(last_index, v);
  }

protected:
  typedef typename convert_type_list_to_tuple<
      typename map_collect_list_to_storage_list<
          util::type_list<ARGS...>>::type>::type content_type;

  content_type m_data;
}; // end nt_layout_storage

template <typename Layout> struct get_layout_storage_type {};
template <typename... ARGS> struct get_layout_storage_type<nt_layout<ARGS...>> {
  typedef nt_layout_storage<ARGS...> type;
};

template <typename NTObjType> struct ntobj_to_ntcollect {};
template <typename... ARGS> struct ntobj_to_ntcollect<ntobject<ARGS...>> {
  typedef nt_collect<ARGS...> type;
};

template <typename TypeList> struct convert_type_list_to_ntobject {};
template <typename... ARGS>
struct convert_type_list_to_ntobject<type_list<ARGS...>> {
  typedef ntobject<ARGS...> type;
};

template <typename... CollectTypes> struct extract_types_from_collects {
  typedef util::type_list<> type;
};
template <typename CollectT1, typename... CTs>
struct extract_types_from_collects<CollectT1, CTs...> {
  typedef typename util::merge_type_list<
      typename CollectT1::type_list,
      typename extract_types_from_collects<CTs...>::type>::type type;
};

template <typename... CollectTypes> struct get_ntobject_type_from_collects {
  typedef typename convert_type_list_to_ntobject<
      typename extract_types_from_collects<CollectTypes...>::type>::type type;
};

} // namespace internal

template <typename NTObjType,
          typename Layout =
              nt_layout<typename internal::ntobj_to_ntcollect<NTObjType>::type>>
class ntcompact_array {
public:
  typedef typename internal::get_layout_storage_type<Layout>::type storage_type;
  typedef ntcompact_array<NTObjType, Layout> self_type;

  template <typename... ARGS> class ntobject_in_array_impl {
  public:
    using owner_type = self_type *;
    ntobject_in_array_impl(size_t index, owner_type s)
        : m_owner(s), m_index(index) {}

    template <typename CT, typename... CARGS, typename... PARGS>
    void set(const typename internal::nt_traits<CT>::type &val,
             PARGS... params) {
      m_owner->m_storage.template set<CT, CARGS...>(m_index, val, params...);
    }

    template <typename CT> typename internal::nt_traits<CT>::type get() const {
      return m_owner->m_storage.template get<CT>(m_index);
    }

  private:
    owner_type m_owner;
    size_t m_index;
  };

  template <typename... ARGS> class const_ntobject_in_array_impl {
  public:
    using owner_type = const self_type *;
    const_ntobject_in_array_impl(size_t index, owner_type s)
        : m_owner(s), m_index(index) {}

    template <typename CT> typename internal::nt_traits<CT>::type get() const {
      return m_owner->m_storage.template get<CT>(m_index);
    }

  private:
    owner_type m_owner;
    size_t m_index;
  };

  template <typename NT> struct ntobject_in_array_type_helper {};
  template <typename... ARGS>
  struct ntobject_in_array_type_helper<ntobject<ARGS...>> {
    typedef ntobject_in_array_impl<ARGS...> type;
    typedef const_ntobject_in_array_impl<ARGS...> ctype;
  };
  typedef typename ntobject_in_array_type_helper<NTObjType>::type
      ntobject_ref_in_array;
  typedef typename ntobject_in_array_type_helper<NTObjType>::ctype
      ntobject_cref_in_array;

  template <typename RefType>
  class iterator_impl
      : public std::iterator<std::random_access_iterator_tag, RefType> {
  public:
    using difference_type =
        typename std::iterator<std::random_access_iterator_tag,
                               ntobject_ref_in_array>::difference_type;
    using value_type = RefType;
    using owner_type = typename RefType::owner_type;

    iterator_impl() : m_owner(nullptr), _ptr(0) {}
    iterator_impl(owner_type owner, size_t pos) : m_owner(owner), _ptr(pos) {}
    iterator_impl(const iterator_impl &rhs)
        : m_owner(rhs.m_owner), _ptr(rhs._ptr) {}
    iterator_impl<RefType> &operator=(const iterator_impl &rhs) {
      if (&rhs == this)
        return *this;
      m_owner = rhs.m_owner;
      _ptr = rhs._pt;
      return *this;
    }
    iterator_impl<RefType> &operator+=(difference_type rhs) {
      _ptr += rhs;
      return *this;
    }
    iterator_impl<RefType> &operator-=(difference_type rhs) {
      _ptr -= rhs;
      return *this;
    }
    value_type operator*() const { return value_type(_ptr, m_owner); }
    // inline Type *operator->() const { return _ptr; }
    value_type operator[](difference_type rhs) const {
      return value_type(_ptr + rhs, m_owner);
    }

    iterator_impl<RefType> &operator++() {
      ++_ptr;
      return *this;
    }
    iterator_impl<RefType> &operator--() {
      --_ptr;
      return *this;
    }
    iterator_impl<RefType> operator++(int) {
      iterator_impl<RefType> tmp(*this);
      ++_ptr;
      return tmp;
    }

    inline iterator_impl<RefType> operator--(int) {
      iterator_impl<RefType> tmp(*this);
      --_ptr;
      return tmp;
    }
    /* inline Iterator operator+(const Iterator& rhs) {return
     * Iterator(_ptr+rhs.ptr);} */
    inline difference_type operator-(const iterator_impl<RefType> &rhs) const {
      return _ptr - rhs._ptr;
    }
    inline iterator_impl<RefType> operator+(difference_type rhs) const {
      return iterator_impl<RefType>(m_owner, _ptr + rhs);
    }
    inline iterator_impl<RefType> operator-(difference_type rhs) const {
      return iterator_impl<RefType>(m_owner, _ptr - rhs);
    }

    inline bool operator==(const iterator_impl<RefType> &rhs) const {
      return _ptr == rhs._ptr && m_owner == rhs.m_owner;
    }
    inline bool operator!=(const iterator_impl<RefType> &rhs) const {
      return _ptr != rhs._ptr || m_owner != rhs.m_owner;
    }
    inline bool operator>(const iterator_impl<RefType> &rhs) const {
      return _ptr > rhs._ptr;
    }
    inline bool operator<(const iterator_impl<RefType> &rhs) const {
      return _ptr < rhs._ptr;
    }
    inline bool operator>=(const iterator_impl<RefType> &rhs) const {
      return _ptr >= rhs._ptr;
    }
    inline bool operator<=(const iterator_impl<RefType> &rhs) const {
      return _ptr <= rhs._ptr;
    }

  private:
    self_type *m_owner;
    size_t _ptr;
  };

  typedef iterator_impl<ntobject_ref_in_array> iterator;
  typedef iterator_impl<ntobject_cref_in_array> const_iterator;

  ntobject_ref_in_array operator[](size_t i) {
    return ntobject_ref_in_array(i, this);
  }
  ntobject_cref_in_array operator[](size_t i) const {
    return ntobject_cref_in_array(i, this);
  }

  ntobject_ref_in_array at(size_t i) {
    if (i >= size()) {
      throw std::runtime_error("ntcompact_array out of range");
    }
    return ntobject_ref_in_array(i, this);
  }

  ntobject_cref_in_array at(size_t i) const {
    if (i >= size()) {
      throw std::runtime_error("ntcompact_array out of range");
    }
    return ntobject_cref_in_array(i, this);
  }

  ntobject_cref_in_array front() const {
    return ntobject_cref_in_array(0, this);
  }
  ntobject_ref_in_array front() { return ntobject_ref_in_array(0, this); }

  ntobject_cref_in_array back() const {
    return ntobject_cref_in_array(m_storage.size() - 1, this);
  }
  ntobject_ref_in_array back() {
    return ntobject_ref_in_array(m_storage.size() - 1, this);
  }

  size_t size() const { return m_storage.size(); }
  bool empty() const { return m_storage.empty(); }
  void clear() { m_storage.clear(); }

  iterator begin() { return iterator(this, 0); }
  const_iterator begin() const { return const_iterator(this, 0); }

  iterator end() { return iterator(this, size()); }
  const_iterator end() const { return const_iterator(this, size()); }

  template <typename... CARGS, typename... PARGS>
  void push_back(PARGS... params) {
    m_storage.template push_back_with_elements<CARGS...>(params...);
  };
  template <typename PType> void push_back(const PType &val) {
    m_storage.push_back_with_ntobject(val);
  }
  void pop_back() { m_storage.pop_back(); }

  template <typename... CollectTypes>
  ntcompact_array<
      typename internal::get_ntobject_type_from_collects<CollectTypes...>::type,
      nt_layout<CollectTypes...>>
  reshape() {
    typedef ntcompact_array<typename internal::get_ntobject_type_from_collects<
                                CollectTypes...>::type,
                            nt_layout<CollectTypes...>>
        new_array_t;
    new_array_t na;

    for (auto it = begin(); it != end(); it++) {
      na.push_back(*it);
    }

    return na;
  }
  template <typename CollectType>
  auto collect() ->
      typename internal::get_collect_storage_type<CollectType>::type::data_type
          & {
    return m_storage.template collect<CollectType>();
  }
  template <typename CollectType>
  auto collect() const -> const
      typename internal::get_collect_storage_type<CollectType>::type::data_type
          & {
    return m_storage.template collect<CollectType>();
  }

protected:
  template <typename CT>
  void set(int index, const typename internal::nt_traits<CT>::type &val) {
    m_storage.template set<CT>(index, val);
  }

protected:
  storage_type m_storage;
};

} // namespace util
} // namespace ff
