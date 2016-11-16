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

#ifndef OUTPUT_TABLES_HPP
#define OUTPUT_TABLES_HPP

#include <vector>

#include <ert/ecl/FortIO.hpp>
#include <ert/ecl/EclKW.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/PvtoTable.hpp>


namespace Opm {
    class UnitSystem;

    class Tables {
    public:
        Tables( const UnitSystem& units_);
        void fwrite( ERT::FortIO& fortio ) const;
        void addPVTO( const std::vector<PvtoTable>& pvtoTables);


    private:
        void addData( size_t offset_index , const std::vector<double>& new_data);

        const UnitSystem&  units;
        ERT::EclKW<int> tabdims;
        std::vector<double> data;
    };
}


#endif
