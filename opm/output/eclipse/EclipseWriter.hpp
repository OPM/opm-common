/*
  Copyright (c) 2013 Andreas Lauser
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2014 IRIS AS

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

#ifndef OPM_ECLIPSE_WRITER_HPP
#define OPM_ECLIPSE_WRITER_HPP

#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>

#include <string>
#include <vector>
#include <array>
#include <memory>

#include <opm/output/Cells.hpp>
#include <opm/output/Wells.hpp>

namespace Opm {

class EclipseState;

/*!
 * \brief A class to write the reservoir state and the well state of a
 *        blackoil simulation to disk using the Eclipse binary format.
 */
class EclipseWriter {
public:
    /*!
     * \brief Sets the common attributes required to write eclipse
     *        binary files using ERT.
     */
    EclipseWriter(std::shared_ptr< const EclipseState >,
                  int numCells,
                  const int* compressedToCartesianCellIdx,
                  const NNC& );


    /**
     * Write the static eclipse data (grid, PVT curves, etc) to disk.
     *
     * If NNC is given to the constructor, writes TRANNNC keyword.
     */
    void writeInit(const std::vector<data::CellData>& simProps = {});

    /*!
     * \brief Write a reservoir state and summary information to disk.
     *
     *
     * The reservoir state can be inspected with visualization tools like
     * ResInsight.
     *
     * The summary information can then be visualized using tools from
     * ERT or ECLIPSE. Note that calling this method is only
     * meaningful after the first time step has been completed.
     */
    void writeTimeStep( int report_step,
                        double seconds_elapsed,
                        data::Solution,
                        data::Wells,
                        bool isSubstep);

    EclipseWriter( const EclipseWriter& ) = delete;
    ~EclipseWriter();

private:
    void writeINITFile(const std::vector<data::CellData>& simProps) const;
    void writeEGRIDFile( ) const;

    class Impl;
    std::unique_ptr< Impl > impl;

};

} // namespace Opm


#endif // OPM_ECLIPSE_WRITER_HPP
