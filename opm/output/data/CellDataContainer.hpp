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

#ifndef OPM_OUTPUT_CELL_DATA_CONTAINER_HPP
#define OPM_OUTPUT_CELL_DATA_CONTAINER_HPP

#include <string>
#include <vector>
#include <initializer_list>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/output/data/Cells.hpp>


namespace Opm {

    /*
      The CelDataContainer class is a small class whose only purpose
      is to transport cell data, i.e. pressure, saturations and
      auxillary properties like fluid in place from the simulator to
      output layer.

      The container consist of instances of struct data::CellData.
    */

    class CellDataContainer {
    public:
        /*
          The initializer_list based constructor can be used as:

          CellDataContainer cd = {{ "PRESSURE" , UnitSystem::measure::pressure , pressure_data , true},
                                  { "SWAT" ,  UnitSystem::measure::unity , swat_data , true}};
        */
        CellDataContainer( std::initializer_list<data::CellData> init_list );
        CellDataContainer( std::vector<data::CellData> init_list );

        /*
          Default constructor - create a valid empty container.
        */
        CellDataContainer( ) = default;

        size_t size() const;
        bool hasKeyword(const std::string& keyword) const;
        const data::CellData& getKeyword(const std::string& keyword) const;

        /*
          Construct a struct data::CellData instance based on the
          input arguments and insert it in the container.
        */
        void insert(const std::string& keyword, UnitSystem::measure dim, const std::vector<double>& data , bool enable_in_restart = true);
        void insert(data::CellData cell_data);

        /*
          Iterate over the struct data::CellData instances in the container.
        */
        std::vector<data::CellData>::const_iterator begin() const ;
        std::vector<data::CellData>::const_iterator end() const;
    private:
        std::vector<data::CellData> data;
    };

}


#endif
