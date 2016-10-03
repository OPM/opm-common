#ifndef ECLIPSEREADER_HPP
#define ECLIPSEREADER_HPP

#include <utility>

#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Wells.hpp>

namespace Opm {

    class EclipseState;

///
/// \brief init_from_restart_file
///     Reading from the restart file, information stored under the OPM_XWEL keyword and SOLUTION data is in this method filled into
///     an instance of a wellstate object and a SimulatorState object.
/// \param grid
///     UnstructuredGrid reference
/// \param pu
///     PhaseUsage reference
/// \param simulator_state
///     An instance of a SimulatorState object
/// \param wellstate
///     An instance of a WellState object, with correct size for each of the 5 contained std::vector<double> objects
///

    std::pair< data::Solution, data::Wells >
    init_from_restart_file( const EclipseState&, int numcells );


}

#endif // ECLIPSEREADER_HPP
