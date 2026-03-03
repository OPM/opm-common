/*
  Copyright 2023 SINTEF Digital

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
#ifndef OPM_TIMINGMACROS_HPP
#define OPM_TIMINGMACROS_HPP

#include <cstdint>

// This file defines macros
// OPM_TIMEBLOCK - time block of main part of codes which do not effect performance
// OPM_TIMEFUNCTION - time block of main part of codes which do not effect performance with name from function
// OPM_TIMEBLOCK_LOCAL - detailed timing which may effect performance
// OPM_TIMEFUNCTION_LOCAL - detailed timing which may effect performance with name from function
#if USE_TRACY
#if USE_TRACY_LOCAL
#define DETAILED_PROFILING 1
#endif
#endif

namespace Opm::Subsystem
{
// If more granularity is needed, add more members to the enum below,
// but keep in mind to possibly increase the number of bits used in
// the representation, as well as updating AnySystem to be just 1-bits
// for the new representation.
enum Bitfield : std::uint8_t
{
    None = 0,
    PvtProps = 1 << 0,
    SatProps = 1 << 1,
    Assembly = 1 << 2,
    LinearSolver = 1 << 3,
    Output = 1 << 4,
    Wells = 1 << 5,
    Other = 1 << 6,   // Consider expanding with more options instead.
    AnySystem = 0xff  // Matches any system.
};
} // namespace Opm::SubSystem

#ifndef DETAILED_PROFILING
#define DETAILED_PROFILING 0 // Turn detailed profiling off.
// #define DETAILED_PROFILING 1 // Turn detailed profiling on.
// The value of DETAILED_PROFILING_SUBSYSTEMS controls which subsystems are
// active for detailed profiling. Some examples below.
// #define DETAILED_PROFILING_SUBSYSTEMS (Opm::Subsystem::PvtProps | Opm::Subsystem::SatProps)
// #define DETAILED_PROFILING_SUBSYSTEMS (Opm::Subsystem::Assembly)
#endif


#if USE_TRACY
#define TRACY_ENABLE 1
#include <tracy/Tracy.hpp>
#define OPM_TIMEBLOCK(blockname) ZoneNamedN(blockname, #blockname, true)
#define OPM_TIMEFUNCTION() ZoneNamedN(myname, __func__, true)
#if DETAILED_PROFILING
#define OPM_TIMEBLOCK_LOCAL(blockname, subsys) ZoneNamedN(blockname, #blockname, DETAILED_PROFILING_SUBSYSTEMS & subsys)
#define OPM_TIMEFUNCTION_LOCAL(subsys) ZoneNamedN(myname, __func__, DETAILED_PROFILING_SUBSYSTEMS & subsys)
#endif
#endif

#ifndef OPM_TIMEBLOCK
#define OPM_TIMEBLOCK(x)\
    do { /* nothing */ } while (false)
#endif

// detailed timing which may effect performance
#ifndef OPM_TIMEBLOCK_LOCAL
#define OPM_TIMEBLOCK_LOCAL(x, subsys)\
    do { /* nothing */ } while (false)
#endif

#ifndef OPM_TIMEFUNCTION
#define OPM_TIMEFUNCTION()\
    do { /* nothing */ } while (false)
#endif

#ifndef OPM_TIMEFUNCTION_LOCAL
#define OPM_TIMEFUNCTION_LOCAL(subsys)\
    do { /* nothing */ } while (false)
#endif

#endif // OPM_TIMINGMACROS_HPP
