/*
 * This code is based on lagrange/modules/core/include/lagrange/Edge.h from Adobe's lagrange project.
 * Author: Chrystiano Araujo (araujoc@cs.ubc.ca)
 *
 * The original code can be found at: https://github.com/adobe/lagrange
 *
 * Copyright 2020 Adobe. All rights reserved.
 * This file is licensed to you under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 * OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */
#pragma once

#include <array>
#include <exception>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace ca_essentials {
namespace core {

class UnorderedIntPair
{
private:
    std::array<int, 2> m_vals;

public:
    UnorderedIntPair(int v1, int v2)
    : m_vals{v1, v2}
    {}
    UnorderedIntPair(const UnorderedIntPair& obj)
    : m_vals(obj.m_vals)
    {}
    UnorderedIntPair(UnorderedIntPair&& obj) noexcept
    : m_vals(obj.m_vals)
    {}
    ~UnorderedIntPair() noexcept = default;

    inline int v1() const { return m_vals[0]; }
    inline int v2() const { return m_vals[1]; }

    UnorderedIntPair& operator=(const UnorderedIntPair& other)
    {
        m_vals = other.m_vals;
        return *this;
    }

    UnorderedIntPair& operator=(UnorderedIntPair&& other) noexcept
    {
        m_vals = other.m_vals;
        return *this;
    }

    bool operator==(const UnorderedIntPair& rhs) const
    {
        return (v1() == rhs.v1() && v2() == rhs.v2()) || (v1() == rhs.v2() && v2() == rhs.v1());
    }

    bool operator!=(const UnorderedIntPair& rhs) const { return !(*this == rhs); }

    int operator[](const int i) const
    {
        if ((i == 0) || (i == 1)) return m_vals[i];
        return v1();
    }

    // Note: strict order operator< is NOT implemented, because:
    // - order of edges is not well defined
    // - by not implementing it, defining a std::set<UnorderedIntPair> will
    //   result in a compile error, and you should *not* use
    //   std::set<UnorderedIntPair>. Use std::unordered_set<UnorderedIntPair> instead.

    // allows: for (Index v : edge) { ... }
    class iterator : std::iterator<std::input_iterator_tag, UnorderedIntPair>
    {
    private:
        int m_i;
        const UnorderedIntPair& m_pair;

    public:
        iterator(const UnorderedIntPair& pair, int i)
            : m_i(i)
            , m_pair(pair)
        {}
        bool operator!=(const iterator& rhs) const
        {
            return m_pair != rhs.m_pair || m_i != rhs.m_i;
        }
        iterator& operator++()
        {
            ++m_i;
            return *this;
        }
        int operator*() const { return m_pair[m_i]; }
    };

    iterator begin() const { return iterator(*this, 0); }
    iterator end() const { return iterator(*this, 2); }
};

};
};

namespace std {
    template <>
    struct hash<ca_essentials::core::UnorderedIntPair>
    {
        std::size_t operator()(const ca_essentials::core::UnorderedIntPair& key) const
        {
            return static_cast<std::size_t>(key.v1() + key.v2());
        }
    };
} // namespace std
