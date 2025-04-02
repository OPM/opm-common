/*
  Copyright (c) 2020 Equinor ASA

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

#ifndef OPM_WRITE_RPT_HPP
#define OPM_WRITE_RPT_HPP

#include <cstddef>
#include <iosfwd>
#include <string>

namespace Opm {

    class Schedule;
    class EclipseGrid;
    class UnitSystem;

} // namespace Opm

namespace Opm::RptIO {

    void write_report(std::ostream&      os,
                      const std::string& reportType,
                      int                reportSpec,
                      const Schedule&    schedule,
                      const EclipseGrid& grid,
                      const UnitSystem&  unit_system,
                      std::size_t        time_step);

} // namespace Opm::RptIO

namespace Opm::RptIO::workers {

    void wellSpecification(std::ostream&      os,
                           int                wellSpecRequest,
                           const Schedule&    schedule,
                           const EclipseGrid& grid,
                           const UnitSystem&  unit_system,
                           std::size_t        time_step);

} // namespace Opm::RptIO::workers

#endif // OPM_WRITE_RPT_HPP
