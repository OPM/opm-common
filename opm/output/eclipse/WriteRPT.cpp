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

#include <opm/output/eclipse/WriteRPT.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace {

    using ReportHandler = std::function<void(std::ostream&,
                                             unsigned,
                                             const Opm::Schedule&,
                                             const Opm::EclipseGrid&,
                                             const Opm::UnitSystem&,
                                             std::size_t)>;

    std::optional<ReportHandler>
    findReportHandler(const std::string& reportType)
    {
        if (reportType == "WELSPECS") {
            return {
                ReportHandler { Opm::RptIO::workers::wellSpecification }
            };
        }

        return {};
    }

} // Anonymous namespace

namespace Opm::RptIO {

    void write_report(std::ostream&      os,
                      const std::string& reportType,
                      const int          reportSpec,
                      const Schedule&    schedule,
                      const EclipseGrid& grid,
                      const UnitSystem&  unit_system,
                      const std::size_t  report_step)
    {
        const auto handler = findReportHandler(reportType);
        if (! handler.has_value()) {
            return;
        }

        std::invoke(*handler, os, reportSpec,
                    schedule, grid,
                    unit_system, report_step);
    }

} // namespace Opm::RptIO
