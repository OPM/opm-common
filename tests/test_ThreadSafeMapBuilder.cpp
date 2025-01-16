/*
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

#include <config.h>

#define BOOST_TEST_MODULE THREAD_SAFE_MAP_BUILDER_TESTS
#include <boost/test/unit_test.hpp>

#include <opm/common/utility/ThreadSafeMapBuilder.hpp>

#include <map>
#include <unordered_map>

#if _OPENMP

namespace {

std::vector<int> rangeVector(const int num_chunks, const int chunk_size)
{
    std::vector<int> ranges(num_chunks + 1);
    for (int i = 0; i < num_chunks; ++i) {
        ranges[i+1] = chunk_size*(i+1);
    }

    return ranges;
}

}

BOOST_AUTO_TEST_CASE(OrderedSimple)
{
    auto ref = std::map<int,int>{};
    for (int i = 0; i < 4 * omp_get_max_threads(); ++i) {
        ref.insert_or_assign(i, i);
    }

    decltype(ref) map1_;
    Opm::ThreadSafeMapBuilder map1(map1_, 1,
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
    const auto ranges = rangeVector(omp_get_max_threads(), 4);

    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map1.insert_or_assign(i, i);
        }
    }
    map1.finalize();

    decltype(ref) map2_;
    Opm::ThreadSafeMapBuilder map2(map2_, omp_get_max_threads(),
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
#pragma omp parallel for
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map2.insert_or_assign(i, i);
        }
    }
    map2.finalize();

    BOOST_CHECK_EQUAL(map1_.size(), ref.size());
    BOOST_CHECK_EQUAL(map2_.size(), ref.size());
    for (auto it1 = map1_.begin(),
              it2 = map2_.begin(),
              rit = ref.begin(); it1 != map1_.end(); ++it1, ++it2, ++rit) {
        BOOST_CHECK_EQUAL(it1->first, rit->first);
        BOOST_CHECK_EQUAL(it2->first, rit->first);
        BOOST_CHECK_EQUAL(it1->second, rit->second);
        BOOST_CHECK_EQUAL(it2->second, rit->second);
    }
}

BOOST_AUTO_TEST_CASE(OrderedComplexKey)
{
    auto ref = std::map<std::pair<int,int>,int>{};
    for (int i = 0; i < 4 * omp_get_max_threads(); ++i) {
        ref.insert_or_assign(std::make_pair(i,i), i);
    }

    decltype(ref) map1_;
    Opm::ThreadSafeMapBuilder map1(map1_, 1,
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
    const auto ranges = rangeVector(omp_get_max_threads(), 4);

    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map1.insert_or_assign(std::make_pair(i,i), i);
        }
    }
    map1.finalize();

    decltype(ref) map2_;
    Opm::ThreadSafeMapBuilder map2(map2_, omp_get_max_threads(),
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
#pragma omp parallel for
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map2.insert_or_assign(std::make_pair(i,i), i);
        }
    }
    map2.finalize();

    BOOST_CHECK_EQUAL(map1_.size(), ref.size());
    BOOST_CHECK_EQUAL(map2_.size(), ref.size());
    for (auto it1 = map1_.begin(),
              it2 = map2_.begin(),
              rit = ref.begin(); it1 != map1_.end(); ++it1, ++it2, ++rit) {
        BOOST_CHECK(it1->first ==  rit->first);
        BOOST_CHECK(it2->first == rit->first);
        BOOST_CHECK(it1->second == rit->second);
        BOOST_CHECK(it2->second == rit->second);
    }
}

BOOST_AUTO_TEST_CASE(OrderedComplexValue)
{
    auto ref = std::map<int,std::pair<int,int>>{};
    for (int i = 0; i < 4 * omp_get_max_threads(); ++i) {
        ref.insert_or_assign(i, std::make_pair(i,i));
    }

    decltype(ref) map1_;
    Opm::ThreadSafeMapBuilder map1(map1_, 1,
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
    const auto ranges = rangeVector(omp_get_max_threads(), 4);

    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map1.insert_or_assign(i, std::make_pair(i,i));
        }
    }
    map1.finalize();

    decltype(ref) map2_;
    Opm::ThreadSafeMapBuilder map2(map2_, omp_get_max_threads(),
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
#pragma omp parallel for
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map2.insert_or_assign(i, std::make_pair(i,i));
        }
    }
    map2.finalize();

    BOOST_CHECK_EQUAL(map1_.size(), ref.size());
    BOOST_CHECK_EQUAL(map2_.size(), ref.size());
    for (auto it1 = map1_.begin(),
              it2 = map2_.begin(),
              rit = ref.begin(); it1 != map1_.end(); ++it1, ++it2, ++rit) {
        BOOST_CHECK(it1->first ==  rit->first);
        BOOST_CHECK(it2->first == rit->first);
        BOOST_CHECK(it1->second == rit->second);
        BOOST_CHECK(it2->second == rit->second);
    }
}

BOOST_AUTO_TEST_CASE(UnorderedSimple)
{
    auto ref = std::unordered_map<int,int>{};
    for (int i = 0; i < 4 * omp_get_max_threads(); ++i) {
        ref.insert_or_assign(i, i);
    }

    decltype(ref) map1_;
    Opm::ThreadSafeMapBuilder map1(map1_, 1,
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
    const auto ranges = rangeVector(omp_get_max_threads(), 4);

    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map1.insert_or_assign(i, i);
        }
    }
    map1.finalize();

    decltype(ref) map2_;
    Opm::ThreadSafeMapBuilder map2(map2_, omp_get_max_threads(),
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
#pragma omp parallel for
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map2.insert_or_assign(i, i);
        }
    }
    map2.finalize();

    BOOST_CHECK_EQUAL(map1_.size(), ref.size());
    BOOST_CHECK_EQUAL(map2_.size(), ref.size());
    for (const auto& rit : ref) {
        const auto it1 = map1_.find(rit.first);
        const auto it2 = map2_.find(rit.first);
        BOOST_CHECK_EQUAL(it1->first, rit.first);
        BOOST_CHECK_EQUAL(it2->first, rit.first);
        BOOST_CHECK_EQUAL(it1->second, rit.second);
        BOOST_CHECK_EQUAL(it2->second, rit.second);
    }
}

BOOST_AUTO_TEST_CASE(UnorderedComplexValue)
{
    auto ref = std::unordered_map<int,std::pair<int,int>>{};
    for (int i = 0; i < 4 * omp_get_max_threads(); ++i) {
        ref.insert_or_assign(i, std::make_pair(i,i));
    }

    decltype(ref) map1_;
    Opm::ThreadSafeMapBuilder map1(map1_, 1,
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
    const auto ranges = rangeVector(omp_get_max_threads(), 4);

    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map1.insert_or_assign(i, std::make_pair(i,i));
        }
    }
    map1.finalize();

    decltype(ref) map2_;
    Opm::ThreadSafeMapBuilder map2(map2_, omp_get_max_threads(),
                                   Opm::MapBuilderInsertionMode::Insert_Or_Assign);
#pragma omp parallel for
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
            map2.insert_or_assign(i, std::make_pair(i,i));
        }
    }
    map2.finalize();

    BOOST_CHECK_EQUAL(map1_.size(), ref.size());
    BOOST_CHECK_EQUAL(map2_.size(), ref.size());
    for (const auto& rit : ref) {
        const auto it1 = map1_.find(rit.first);
        const auto it2 = map2_.find(rit.first);
        BOOST_CHECK(it1->first == rit.first);
        BOOST_CHECK(it2->first ==  rit.first);
        BOOST_CHECK(it1->second ==  rit.second);
        BOOST_CHECK(it2->second == rit.second);
    }
}

BOOST_AUTO_TEST_CASE(OrderedSimpleEmplace)
{
    auto ref = std::map<int,int>{};
    for (int i = 0; i < 4 * omp_get_max_threads(); ++i) {
        ref.insert_or_assign(i, i);
    }

    decltype(ref) map1_;
    Opm::ThreadSafeMapBuilder map1(map1_, 1,
                                   Opm::MapBuilderInsertionMode::Emplace);
    const auto ranges = rangeVector(omp_get_max_threads(), 4);

    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
      for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
          map1.emplace(i, i);
          map1.emplace(i, 2*i);
      }
    }
    map1.finalize();

    decltype(ref) map2_;
    Opm::ThreadSafeMapBuilder map2(map2_, omp_get_max_threads(),
                                   Opm::MapBuilderInsertionMode::Emplace);
#pragma omp parallel for
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
      for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
          map2.emplace(i, i);
          map2.emplace(i, 2*i);
      }
    }
    map2.finalize();

    BOOST_CHECK_EQUAL(map1_.size(), ref.size());
    BOOST_CHECK_EQUAL(map2_.size(), ref.size());
    for (const auto& rit : ref) {
        const auto it1 = map1_.find(rit.first);
        const auto it2 = map2_.find(rit.first);
        BOOST_CHECK_EQUAL(it1->first, rit.first);
        BOOST_CHECK_EQUAL(it2->first, rit.first);
        BOOST_CHECK_EQUAL(it1->second, rit.second);
        BOOST_CHECK_EQUAL(it2->second, rit.second);
    }
}

BOOST_AUTO_TEST_CASE(UnorderedSimpleEmplace)
{
    auto ref = std::unordered_map<int,int>{};
    for (int i = 0; i < 4 * omp_get_max_threads(); ++i) {
        ref.insert_or_assign(i, i);
    }

    decltype(ref) map1_;
    Opm::ThreadSafeMapBuilder map1(map1_, 1,
                                   Opm::MapBuilderInsertionMode::Emplace);
    const auto ranges = rangeVector(omp_get_max_threads(), 4);

    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
      for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
          map1.emplace(i, i);
          map1.emplace(i, 2*i);
      }
    }
    map1.finalize();

    decltype(ref) map2_;
    Opm::ThreadSafeMapBuilder map2(map2_, omp_get_max_threads(),
                                   Opm::MapBuilderInsertionMode::Emplace);
#pragma omp parallel for
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
      for (int i = ranges[chunk]; i < ranges[chunk+1]; ++i) {
          map2.emplace(i, i);
          map2.emplace(i, 2*i);
      }
    }
    map2.finalize();

    BOOST_CHECK_EQUAL(map1_.size(), ref.size());
    BOOST_CHECK_EQUAL(map2_.size(), ref.size());
    for (const auto& rit : ref) {
        const auto it1 = map1_.find(rit.first);
        const auto it2 = map2_.find(rit.first);
        BOOST_CHECK_EQUAL(it1->first, rit.first);
        BOOST_CHECK_EQUAL(it2->first, rit.first);
        BOOST_CHECK_EQUAL(it1->second, rit.second);
        BOOST_CHECK_EQUAL(it2->second, rit.second);
    }
}

#endif // _OPENMP
