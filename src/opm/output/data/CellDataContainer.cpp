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

#include <opm/output/data/CellDataContainer.hpp>
#include <opm/output/data/Cells.hpp>


namespace Opm {

    CellDataContainer::CellDataContainer( std::initializer_list<data::CellData> init_list )
        : data( init_list )
    { }


    CellDataContainer::CellDataContainer( std::vector<data::CellData> init_list )
        : data(std::move(init_list))
    { }



    size_t CellDataContainer::size() const {
        return this->data.size();
    }


    bool CellDataContainer::hasKeyword(const std::string& keyword) const {
        const auto iter = std::find_if( this->data.begin() , this->data.end() , [&keyword](const data::CellData& cd) { return cd.name == keyword; });
        return (iter != this->data.end());
    }

    void CellDataContainer::insert(const std::string& keyword, UnitSystem::measure dim, const std::vector<double>& data , bool enable_in_restart) {
        data::CellData cd { keyword, dim , data , enable_in_restart };
        this->insert( cd );
    }


    void CellDataContainer::insert(data::CellData cell_data) {
        this->data.push_back( std::move(cell_data) );
    }


    const data::CellData& CellDataContainer::getKeyword(const std::string& keyword) const {
        const auto iter = std::find_if( this->data.begin() , this->data.end() , [&keyword](const data::CellData& cd) { return cd.name == keyword; });
        if (iter == this->data.end())
            throw std::invalid_argument("No such keyword in container: " + keyword);
        else
            return *iter;
    }




    std::vector<data::CellData>::const_iterator CellDataContainer::begin() const {
        return this->data.begin();
    }


    std::vector<data::CellData>::const_iterator CellDataContainer::end() const {
        return this->data.end();
    }

}


