/*
  This file is part of the Open Porous Media project (OPM).

  Copyright 2025 SINTEF Digital

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

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
#ifndef OPM_THREAD_SAFE_MAP_BUILDER_HPP
#define OPM_THREAD_SAFE_MAP_BUILDER_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <list>
#include <map>
#include <numeric>
#include <vector>

#if _OPENMP
#include <omp.h>
#endif

namespace Opm {

//! \brief Utility class for adding multi-threading to loops that
//!        are building maps.
//! details Intended to be used for building maps in OpenMP loops.
//!         In the parallel section the values are assembled into a
//!         list per thread. After the parallel section the lists are
//!         assembled to the map, either through RAII in the dtor,
//!         or through a manual call to finalize().
//!         Implements both emplace and insert_or_assign, but there are
//!         a need for an assumption. In particular, the assumption is
//!         that iteration loops are ordered such that the first thread
//!         processes the first part of the range. If the order of processing
//!         is unspecified the final map results is not guaranteed to match
//!         for differing number of threads.

//! Available insertion modes.
enum class MapBuilderInsertionMode {
    Emplace,          // Use first inserted value encountered
    Insert_Or_Assign  // Use last inserted value encountered (last thread wins)
};

template<class Map>
class ThreadSafeMapBuilder
{
    //! \brief Predicate for maps
    template<class T>
    struct is_ordered
    {
        constexpr static bool value = false;
    };

    template<class Key, class T, class Compare, class Allocator>
    struct is_ordered<std::map<Key,T,Compare,Allocator>>
    {
        constexpr static bool value = true;
    };

public:
    using Key = typename Map::key_type;
    using Value = typename Map::mapped_type;

    //! \brief Constructor for a given map and number of threads.
    ThreadSafeMapBuilder(Map& map, const int num_threads,
                         const MapBuilderInsertionMode mode)
        : map_(map), num_threads_(num_threads), mode_(mode)
    {
        if (num_threads_ > 1) {
            thread_values_.resize(num_threads_);
        }
    }

    //! \brief The destructor (optionally) assembles the final map.
    ~ThreadSafeMapBuilder()
    {
        finalize();
    }

    //! \brief Insert a value into the map.
    template<class M>
    void insert_or_assign(const Key& key, M&& value)
    {
        assert(mode_ == MapBuilderInsertionMode::Insert_Or_Assign);
#if _OPENMP
        if (num_threads_ > 1) {
            thread_values_[omp_get_thread_num()].
                emplace_back(key, std::forward<M>(value));
        } else
#endif // _OPENMP
        {
            map_.insert_or_assign(key, std::forward<M>(value));
        }
    }

    //! \brief Insert a value into the map.
    template<class... Args>
    void emplace(Args&&... args)
    {
        assert(mode_ == MapBuilderInsertionMode::Emplace);
#if _OPENMP
        if (num_threads_ > 1) {
            thread_values_[omp_get_thread_num()].
                emplace_back(std::forward<Args>(args)...);
        } else
#endif // _OPENMP
        {
            map_.emplace(std::forward<Args>(args)...);
        }
    }

    //! \brief Assembles the final map.
    void finalize()
    {
        if (num_threads_ > 1 && !thread_values_.empty()) {
            if constexpr (!is_ordered<Map>::value) {
                const std::size_t size =
                    std::accumulate(thread_values_.begin(),
                                    thread_values_.end(), std::size_t(0),
                                [](std::size_t s, const auto& v) { return s + v.size(); });
                    map_.reserve(size);
            }
            for (int i = 0; i < num_threads_; ++i) {
                for (const auto& it : thread_values_[i]) {
                    if (mode_== MapBuilderInsertionMode::Insert_Or_Assign) {
                        map_.insert_or_assign(std::move(it.first), std::move(it.second));
                    } else {
                        map_.emplace(std::move(it.first), std::move(it.second));
                    }
                }
            }
        }
        thread_values_.clear();
    }

private:
    std::vector<std::list<std::pair<Key,Value>>> thread_values_;
    Map& map_;
    int num_threads_;
    MapBuilderInsertionMode mode_;
};

}

#endif // OPM_THREAD_SAFE_MAP_BUILDER_HPP
