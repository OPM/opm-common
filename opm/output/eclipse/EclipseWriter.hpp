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
#include <opm/output/OutputWriter.hpp>
#include <opm/output/eclipse/Summary.hpp>


#include <string>
#include <vector>
#include <array>
#include <memory>

namespace Opm {

class EclipseGrid;
class Well;

namespace out {
/*!
 * \brief A class to write the RFT file to disk. Keeps the file handle alive,
 * i.e. you cannot read from the RFT file as long as the instance that wrote it
 * is alive in the same process.
 *
 * You should generally not interact with this component directly, but rather
 * use the higher level interface of EclipseWriter.
 */
class RFT {
    public:
        RFT( const char* output_dir,
             const char* basename,
             bool format,
             const int* compressed_to_cartesian,
             size_t num_cells,
             size_t cartesian_size );

        void writeTimeStep( std::vector< std::shared_ptr< const Well > >,
                            const EclipseGrid& grid,
                            int report_step,
                            time_t current_time,
                            double days,
                            ert_ecl_unit_enum,
                            const std::vector< double >& pressure,
                            const std::vector< double >& swat,
                            const std::vector< double >& sgas );
    private:
        std::vector< int > global_to_active;
        ERT::FortIO fortio;
};

}

/*!
 * \brief A class to write the reservoir state and the well state of a
 *        blackoil simulation to disk using the Eclipse binary format.
 */
class EclipseWriter : public OutputWriter
{
public:
    /*!
     * \brief Sets the common attributes required to write eclipse
     *        binary files using ERT.
     */
    EclipseWriter(std::shared_ptr< const EclipseState >,
                  int numCells,
                  const int* compressedToCartesianCellIdx);

    /**
     * Write the static eclipse data (grid, PVT curves, etc) to disk.
     *
     * If NNC is given, writes TRANNNC keyword.
     */
    virtual void writeInit( time_t current_time, double start_time, const NNC& nnc = NNC() ) override;

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
    virtual void writeTimeStep( int report_step,
                                time_t current_posix_time,
                                double seconds_elapsed,
                                data::Solution,
                                data::Wells,
                                bool isSubstep);

private:
    std::shared_ptr< const EclipseState > eclipseState_;
    std::string outputDir_;
    std::string baseName_;
    out::Summary summary_;
    out::RFT rft_;
    int numCells_;
    std::array<int, 3> cartesianSize_;
    const int* compressedToCartesianCellIdx_;
    std::vector< int > gridToEclipseIdx_;
    const double* conversion_table_;
    bool enableOutput_;
    int ert_phase_mask_;

    void init( std::shared_ptr< const EclipseState > );
};

typedef std::shared_ptr<EclipseWriter> EclipseWriterPtr;
typedef std::shared_ptr<const EclipseWriter> EclipseWriterConstPtr;

} // namespace Opm


#endif // OPM_ECLIPSE_WRITER_HPP
