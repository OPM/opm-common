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

#include <opm/output/eclipse/report/WellSpecification.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace {

    using ReportHandler = std::function<void(std::ostream&,
                                             const unsigned,
                                             const double,
                                             const std::size_t,
                                             const Opm::Schedule&,
                                             const Opm::EclipseGrid&,
                                             const Opm::UnitSystem&)>;

    std::optional<ReportHandler>
    findReportHandler(const std::string& reportType)
    {
        if (reportType == "WELSPECS") {
            return {
                ReportHandler { Opm::PrtFile::Reports::wellSpecification }
            };
        }

        return {};
    }

} // Anonymous namespace

namespace Opm::PrtFile {

    void report(std::ostream&      os,
                const std::string& reportType,
                const int          reportSpec,
                const double       elapsed_secs,
                const std::size_t  report_step,
                const Schedule&    schedule,
                const EclipseGrid& grid,
                const UnitSystem&  unit_system)
    {
        const auto handler = findReportHandler(reportType);
        if (! handler.has_value()) {
            return;
        }

        std::invoke(*handler, os, reportSpec,
                    elapsed_secs, report_step,
                    schedule, grid, unit_system);
    }

} // namespace Opm::RptIO
