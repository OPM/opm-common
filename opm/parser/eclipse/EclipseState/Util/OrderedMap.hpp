/*
  Copyright 2014 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPM_ORDERED_MAP_HPP
#define OPM_ORDERED_MAP_HPP

#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>


namespace Opm {

template <typename KeyType, typename ValueType>
class OrderedMap {
private:
    std::unordered_map<KeyType, size_t> m_map;
    std::vector<std::pair<KeyType, ValueType>> m_vector;

public:

    std::size_t count(const KeyType& key) const {
        return this->m_map.count(key);
    }



    ValueType& operator[](const KeyType& key) {
        if (this->count(key) == 0)
            this->insert( std::make_pair(key, ValueType()));

        return this->at(key);
    }


    void insert(std::pair<KeyType,ValueType> pair) {
        if (this->count(pair.first) > 0) {
            auto iter = m_map.find( pair.first );
            size_t index = iter->second;
            m_vector[index] = pair;
        } else {
            size_t index = m_vector.size();
            m_map.insert( std::pair<std::string, size_t>(pair.first , index));
            m_vector.push_back( std::move( pair ) );
        }
    }


    ValueType& get(const KeyType& key) {
        auto iter = m_map.find( key );
        if (iter == m_map.end())
            throw std::invalid_argument("Key not found:");
        else {
            size_t index = iter->second;
            return iget(index);
        }
    }


    ValueType& iget(size_t index) {
        if (index >= m_vector.size())
            throw std::invalid_argument("Invalid index");
        return m_vector[index].second;
    }

    const ValueType& get(const KeyType& key) const {
        auto iter = m_map.find( key );
        if (iter == m_map.end())
            throw std::invalid_argument("Key not found: ??");
        else {
            size_t index = iter->second;
            return iget(index);
        }
    }


    const ValueType& iget(size_t index) const {
        if (index >= m_vector.size())
            throw std::invalid_argument("Invalid index");
        return m_vector[index].second;
    }

    const ValueType& at(size_t index) const {
        return this->iget(index);
    }

    const ValueType& at(const KeyType& key) const {
        return this->get(key);
    }

    ValueType& at(size_t index) {
        return this->iget(index);
    }

    ValueType& at(const KeyType& key) {
        return this->get(key);
    }

    size_t size() const {
        return m_vector.size();
    }


    typename std::vector<std::pair<KeyType, ValueType>>::const_iterator begin() const {
        return m_vector.begin();
    }


    typename std::vector<std::pair<KeyType, ValueType>>::const_iterator end() const {
        return m_vector.end();
    }

    typename std::vector<std::pair<KeyType, ValueType>>::iterator begin() {
        return m_vector.begin();
    }

    typename std::vector<std::pair<KeyType, ValueType>>::iterator end() {
        return m_vector.end();
    }

    typename std::vector<std::pair<KeyType, ValueType>>::iterator find(const KeyType& key) {
        const auto map_iter = this->m_map.find(key);
        if (map_iter == this->m_map.end())
            return this->m_vector.end();

        return std::next(this->m_vector.begin(), map_iter->second);
    }

    typename std::vector<std::pair<KeyType, ValueType>>::const_iterator find(const KeyType& key) const {
        const auto map_iter = this->m_map.find(key);
        if (map_iter == this->m_map.end())
            return this->m_vector.end();

        return std::next(this->m_vector.begin(), map_iter->second);
    }
};
}

#endif
