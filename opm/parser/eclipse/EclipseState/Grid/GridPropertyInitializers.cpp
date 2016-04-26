/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <limits>
#include <vector>
#include <array>

#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridPropertyInitializers.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RtempvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>


namespace Opm {

    std::vector< double > temperature_lookup( size_t size,
                                              const TableManager* tables,
                                              const EclipseGrid* grid,
                                              GridProperties<int>* ig_props ) {

        if( !tables->useEqlnum() ) {
            /* if values are defaulted in the TEMPI keyword, but no
             * EQLNUM is specified, you will get NaNs
             */
            return std::vector< double >( size, std::numeric_limits< double >::quiet_NaN() );
        }

        std::vector< double > values( size, 0 );

        const auto& rtempvdTables = tables->getRtempvdTables();
        const std::vector< int >& eqlNum = ig_props->getKeyword("EQLNUM").getData();

        for (size_t cellIdx = 0; cellIdx < eqlNum.size(); ++ cellIdx) {
            int cellEquilNum = eqlNum[cellIdx];
            const RtempvdTable& rtempvdTable = rtempvdTables.getTable<RtempvdTable>(cellEquilNum);
            double cellDepth = std::get<2>(grid->getCellCenter(cellIdx));
            values[cellIdx] = rtempvdTable.evaluate("Temperature", cellDepth);
        }

        return values;
    }
}
