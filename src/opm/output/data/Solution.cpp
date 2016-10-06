/*
  Copyright 2016 Statoil ASA.

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

#include <algorithm>

#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Cells.hpp>


namespace Opm {
namespace data {

    Solution::Solution( std::initializer_list<data::CellData> init_list )
        : storage( init_list )
    { }


    Solution::Solution( std::vector<data::CellData> init_list )
        : storage(std::move(init_list))
    { }


    Solution::Solution( bool init_si )
        : si( init_si )
    { }

    size_t Solution::size() const {
        return this->storage.size();
    }


    bool Solution::has(const std::string& keyword) const {
        const auto iter = std::find_if( this->storage.begin() , this->storage.end() , [&keyword](const data::CellData& cd) { return cd.name == keyword; });
        return (iter != this->storage.end());
    }

    void Solution::insert(const std::string& keyword, UnitSystem::measure dim, const std::vector<double>& data , TargetType target) {
        data::CellData cd { keyword, dim , data , target };
        this->insert( cd );
    }


    void Solution::insert(data::CellData cell_data) {
        this->storage.push_back( std::move(cell_data) );
    }


    const data::CellData& Solution::get(const std::string& keyword) const {
        const auto iter = std::find_if( this->storage.begin() , this->storage.end() , [&keyword](const data::CellData& cd) { return cd.name == keyword; });
        if (iter == this->storage.end())
            throw std::invalid_argument("No such keyword in container: " + keyword);
        else
            return *iter;
    }

    data::CellData& Solution::get(const std::string& keyword)  {
        auto iter = std::find_if( this->storage.begin() , this->storage.end() , [&keyword](const data::CellData& cd) { return cd.name == keyword; });
        if (iter == this->storage.end())
            throw std::invalid_argument("No such keyword in container: " + keyword);
        else
            return *iter;
    }

    std::vector<double>& Solution::data(const std::string& keyword) {
        auto& elm = this->get( keyword );
        return elm.data;
    }

    const std::vector<double>& Solution::data(const std::string& keyword) const {
        const auto& elm = this->get( keyword );
        return elm.data;
    }

    void data::Solution::convertToSI( const UnitSystem& units ) {
        if (this->si)
            return;

        for (auto& elm : this->storage) {
            UnitSystem::measure dim = elm.dim;
            if (dim != UnitSystem::measure::identity)
                units.to_si( dim , elm.data );
        }

        this->si = true;
    }

    void data::Solution::convertFromSI( const UnitSystem& units ) {
        if (!this->si)
            return;

        for (auto& elm : this->storage) {
            UnitSystem::measure dim = elm.dim;
            if (dim != UnitSystem::measure::identity)
                units.from_si( dim , elm.data );
        }

        this->si = false;
    }


    std::vector<data::CellData>::const_iterator Solution::begin() const {
        return this->storage.begin();
    }


    std::vector<data::CellData>::const_iterator Solution::end() const {
        return this->storage.end();
    }

}
}

