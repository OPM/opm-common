/*
  Copyright (C) 2018 Statoil ASA

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

#ifndef OPM_PARSER_SIMPLE_2D_TABLE_HPP
#define OPM_PARSER_SIMPLE_2D_TABLE_HPP

// TODO: it is a simple version to begin with, will check whether to introduce
// the Schemas and TableColumn

#include <vector>

namespace Opm {

    class Simple2DTable {
    public:

        int getTableNumber() const;

        const std::vector<double>& getXSamplingPoints() const;
        const std::vector<double>& getYSamplingPoints() const;
        const std::vector<std::vector<double>>& getTableData() const;

    protected:
        std::vector<double> m_x_points;
        std::vector<double> m_y_points;

        // TODO: maybe not needed, since this is also stored in the std::map
        int m_table_number;

        // each vector corresponds to the values corresponds to one value related to one x sampling point
        // as a result, the number of the vector should be equal to be the size of m_x_points,
        // the size of each vector should be equal to the size of m_y_points
        std::vector<std::vector<double> > m_data;
    };
}

#endif // OPM_PARSER_SIMPLE_2D_TABLE_HPP
