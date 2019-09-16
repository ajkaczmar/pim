#pragma once

#include "containers/array.h"

template<typename K, typename V>
struct Dict
{
    Array<K> m_keys;
    Array<V> m_values;

    inline usize size() const { return m_keys.size(); }
    inline usize capacity() const { return m_keys.capacity(); }
    inline bool empty() const { return m_keys.empty(); }
    inline bool full() const { return m_keys.full(); }

    inline void init()
    {
        m_keys.init();
        m_values.init();
    }
    inline void reset()
    {
        m_keys.reset();
        m_values.reset();
    }
    inline void clear()
    {
        m_keys.clear();
        m_values.clear();
    }
    inline isize find(K key) const
    {
        return m_keys.rfind(key);
    }
    inline bool contains(K key) const { return find(key) != -1; }
    inline const V* get(K key) const
    {
        const isize i = find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline V* get(K key)
    {
        const isize i = find(key);
        return (i == -1) ? 0 : m_values.begin() + i;
    }
    inline void remove(K key)
    {
        const isize i = find(key);
        if (i != -1)
        {
            m_keys.remove(i);
            m_values.remove(i);
        }
    }
    inline V& operator[](K key)
    {
        const isize i = find(key);
        if (i != -1)
        {
            return m_values[i];
        }
        else
        {
            m_keys.grow() = key;
            return m_values.grow();
        }
    }
};

template<usize t_width, typename K, typename V>
struct DictTable
{
    static constexpr usize Width = t_width;
    static constexpr usize Mask = t_width - 1u;
    static constexpr usize GetSlot(K key)
    {
        return (key * 16777619) & Mask;
    }

    Dict<K, V> m_dicts[Width];

    inline void init()
    {
        for (auto& dict : m_dicts)
        {
            dict.init();
        }
    }
    inline void reset()
    {
        for (auto& dict : m_dicts)
        {
            dict.reset();
        }
    }
    inline void clear()
    {
        for (auto& dict : m_dicts)
        {
            dict.clear();
        }
    }
    inline isize find(K key) const
    {
        return m_dicts[GetSlot(key)].find(key);
    }
    inline bool contains(K key) const { return find(key) != -1; }
    inline const V* get(K key) const
    {
        return m_dicts[GetSlot(key)].get(key);
    }
    inline V* get(K key)
    {
        return m_dicts[GetSlot(key)].get(key);
    }
    inline void remove(K key)
    {
        m_dicts[GetSlot(key)].remove(key);
    }
    inline V& operator[](K key)
    {
        return m_dicts[GetSlot(key)][key];
    }
};
