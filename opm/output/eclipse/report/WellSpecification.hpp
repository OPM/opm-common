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

#ifndef OPM_REPORT_WELL_SPECIFICATION_HPP_INCLUDED
#define OPM_REPORT_WELL_SPECIFICATION_HPP_INCLUDED

#include <cstddef>
#include <functional>
#include <iosfwd>
#include <string>

namespace Opm {

    class Schedule;

} // namespace Opm

namespace Opm::PrtFile::Reports {

    using BlockDepthCallback = std::function<double(std::size_t)>;

    void wellSpecification(const std::vector<std::string>& changedWells,
                           const std::size_t               reportStep,
                           const Schedule&                 schedule,
                           BlockDepthCallback              blockDepth,
                           std::ostream&                   os);

} // namespace Opm::PrtFile::Reports

#endif // OPM_REPORT_WELL_SPECIFICATION_HPP_INCLUDED
