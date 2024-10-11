#pragma once
#include <iostream>
#include "hpda/hpda.h"
#include "ff/util/ntobject.h"

/**
 * helper class for named processor and helper function for pretty print ntobject
 */
template <typename T>
class named_pp
{
    std::string name;

public:
    named_pp(std::string name) : name(name)
    {
    }
    std::string get_name()
    {
        return name;
    }
    virtual std::string pp(T t);
};

template <typename T>
class raw_data_pp : public hpda::extractor::internal::raw_data_impl<T>,
                    named_pp<T>
{
public:
    raw_data_pp(std::string name = "raw_data_pp")
        : hpda::extractor::internal::raw_data_impl<T>(), named_pp<T>(name)
    {
    }

    bool process() override
    {
        std::cout << named_pp<T>::get_name() << " process() ";
        bool re = hpda::extractor::internal::raw_data_impl<T>::process();
        if (re)
        {
            T temp = hpda::extractor::internal::raw_data_impl<T>::output_value();
            std::cout << "-> success, with output [" << named_pp<T>::pp(temp) << "]" << std::endl;
        }
        else
        {
            std::cout << "-> failed" << std::endl;
        }
        return re;
    }

    // ToDo: consume_input_value()
};

template <typename T>
class concat_pp : public hpda::processor::internal::concat_impl<T>, named_pp<T>
{
public:
    concat_pp(hpda::internal::processor_with_output<T> *upper_stream, std::string name = "concat_pp")
        : hpda::processor::internal::concat_impl<T>(upper_stream), named_pp<T>(name)
    {
    }

    bool process() override
    {
        std::cout << named_pp<T>::get_name() << " process() ";
        bool re = hpda::processor::internal::concat_impl<T>::process();
        if (re)
        {
            T temp = hpda::processor::internal::concat_impl<T>::output_value();
            std::cout << "-> success, with output [" << named_pp<T>::pp(temp) << "]" << std::endl;
        }
        else
        {
            std::cout << "-> failed" << std::endl;
        }
        return re;
    }
};

template <typename T>
class sort
    : public hpda::processor::internal::processor_base<T, T>
{
    bool (*comp)(T &, T &);

public:
    sort(hpda::internal::processor_with_output<T> *upper_stream, bool (*comp)(T &, T &))
        : hpda::processor::internal::processor_base<T, T>(upper_stream),
          comp(comp)
    {
    }

    bool process() override
    {
        if (hpda::processor::internal::processor_base<T, T>::has_input_value())
        {
            m_data.push_back(hpda::processor::internal::processor_base<T, T>::input_value().make_copy());
            hpda::processor::internal::processor_base<T, T>::consume_input_value();
            return false;
        }
        else
        {
            if (!m_sorted)
            {
                do_sort();
            }
            if (m_data.empty())
            {
                return false;
            }
            m_output_idx++;
            if (m_output_idx >= static_cast<int>(m_data.size()))
            {
                return false;
            }
            return true;
        }
    }

    T output_value() override
    {
        return m_data[m_output_idx];
    }

private:
    void do_sort()
    {
        quicksort(m_data, 0, m_data.size() - 1);
    }

    int partition(std::vector<T> &arr, int low, int high)
    {
        T pivot = arr[high];
        int i = low - 1;

        for (int j = low; j <= high - 1; j++)
        {
            if (comp(arr[j], pivot))
            {
                i++;
                T tmp = arr[i].make_copy();
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }

        T tmp = arr[i + 1].make_copy();
        arr[i + 1] = arr[high];
        arr[high] = tmp;
        return i + 1;
    }

    void quicksort(std::vector<T> &arr, int low, int high)
    {
        if (low < high)
        {
            int pi = partition(arr, low, high);
            quicksort(arr, low, pi - 1);
            quicksort(arr, pi + 1, high);
        }
    }

    std::vector<T> m_data;
    bool m_sorted = false;
    uint m_output_idx = -1;
};

template <typename T>
class sort_pp
    : public sort<T>,
      named_pp<T>
{
public:
    sort_pp(hpda::internal::processor_with_output<T> *upper_stream, bool (*comp)(T &, T &), std::string name = "sort_pp")
        : sort<T>(upper_stream, comp), named_pp<T>(name)
    {
    }

    bool process() override
    {
        std::cout << named_pp<T>::get_name() << " process() ";
        bool re = sort<T>::process();
        if (re)
        {
            T temp = sort<T>::output_value();
            std::cout << "-> success, with output [" << named_pp<T>::pp(temp) << "]" << std::endl;
        }
        else
        {
            std::cout << "-> failed" << std::endl;
        }
        return re;
    }
};

template <typename T>
class split_pp : public hpda::processor::internal::split_impl<T>, named_pp<T>
{
public:
    split_pp(hpda::internal::processor_with_output<T> *upper_stream, std::string name = "split_pp")
        : hpda::processor::internal::split_impl<T>(upper_stream), named_pp<T>(name)
    {
    }

    bool process() override
    {
        std::cout << named_pp<T>::get_name() << " process() ";
        bool re = hpda::processor::internal::split_impl<T>::process();
        if (re)
        {
            std::cout << "-> success" << std::endl;
        }
        else
        {
            std::cout << "-> failed" << std::endl;
        }
        return re;
    }
};

template <typename T>
class hash_split_pp : public hpda::processor::internal::split_impl<T>, named_pp<T>
{
std::hash<T> hashFunc;
public:
    hash_split_pp(hpda::internal::processor_with_output<T> *upper_stream, std::string name = "hash_split_pp")
        : hpda::processor::internal::split_impl<T>(upper_stream), named_pp<T>(name)
    {
    }

    bool process() override
    {
        std::cout << named_pp<T>::get_name() << " process() ";
        bool re = _process();
        if (re)
        {
            std::cout << "-> success" << std::endl;
        }
        else
        {
            std::cout << "-> failed" << std::endl;
        }
        return re;
    }
    bool _process()
    {
        if (hpda::processor::internal::split_impl<T>::m_streams.empty() || !hpda::processor::internal::split_impl<T>::has_input_value())
        {
            return false;
        }
        T t = hpda::processor::internal::split_impl<T>::input_value();
        size_t idx = hashFunc(t) % hpda::processor::internal::split_impl<T>::m_streams.size();
        hpda::processor::internal::split_impl<T>::m_streams[idx]->add_data(t.make_copy());
        std::cout << "-> {" << idx << "}-th successor ";
        hpda::processor::internal::split_impl<T>::consume_input_value();
        return true;
    }
};

template <typename T>
class memory_output_pp : public hpda::output::internal::memory_output_impl<T>, named_pp<T>
{
public:
    memory_output_pp(hpda::internal::processor_with_output<T> *upper_stream, std::string name = "memory_output_pp")
        : hpda::output::internal::memory_output_impl<T>(upper_stream), named_pp<T>(name)
    {
    }

    bool process() override
    {
        std::cout << named_pp<T>::get_name() << " process() ";
        bool re = hpda::output::internal::memory_output_impl<T>::process();
        if (re)
        {
            auto val = hpda::output::internal::memory_output_impl<T>::values().back();
            std::cout << "-> success, with output [" << named_pp<T>::pp(val) << "]" << std::endl;
        }
        else
        {
            std::cout << "-> failed" << std::endl;
        }
        return re;
    }
};