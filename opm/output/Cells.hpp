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

#ifndef OPM_OUTPUT_CELLS_HPP
#define OPM_OUTPUT_CELLS_HPP

#include <map>
#include <vector>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

class SimulationDataContainer;

namespace data {

    /**
     * Small struct that keeps track of data for output to restart/summary files.
     */
    struct CellData {
        std::string name;          //< Name of the output field (will end up "verbatim" in output)
        UnitSystem::measure dim;   //< Dimension of the data to write
        std::vector<double> data;  //< The actual data itself
        bool enable_in_restart;    //< Enables writing this field to a restart file.
    };


    struct Solution {
        /* data::Solution supports writing only some information,
         * distinguished by keys. When adding support for more values in
         * the future, add a proper key.
         */
        enum class key {
            PRESSURE,
            TEMP,
            SWAT,
            SGAS,
            RS,
            RV,
        };

        inline bool has( key ) const;

        inline const std::vector< double >& operator[]( key ) const;
        inline std::vector< double >& operator[]( key );

        void insert( key, std::vector< double > );

        /* data::Solution expect the following to assumptions to be true:
         * * vector index corresponds to cell index
         * * all units are SI
         * * cells are active indexed
         */
        std::map< key, std::vector< double > > data;

        /* hack to keep matlab/vtk output support */
        const SimulationDataContainer* sdc = nullptr;
    };

    struct Static {};

    inline bool Solution::has( Solution::key k ) const {
        return this->data.find( k ) != this->data.end();
    }

    inline const std::vector< double >& Solution::operator[]( Solution::key k ) const {
        return this->data.at( k );
    }

    inline std::vector< double >& Solution::operator[]( Solution::key k ) {
        return this->data[ k ];
    }

    inline void Solution::insert( Solution::key k, std::vector< double > v ) {
        this->data.emplace( k, std::move( v ) );
    }
}
}

#endif //OPM_OUTPUT_CELLS_HPP
