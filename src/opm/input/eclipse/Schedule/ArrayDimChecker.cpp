/*
  Copyright (c) 2019, 2023 Equinor ASA

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

#include <config.h>

#include <opm/input/eclipse/Schedule/ArrayDimChecker.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Group/Group.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

#include <fmt/format.h>

namespace {

    void reportError(std::string_view         keyword,
                     const std::size_t        schedVal,
                     const std::size_t        item,
                     std::string_view         entity,
                     const std::string&       ctxtKey,
                     const Opm::ParseContext& ctxt,
                     Opm::ErrorGuard&         guard)
    {
        using namespace fmt::literals;

        // Note: Number of leading blanks on line 2 affects the formatted output.
        const auto message_fmt = fmt::format(R"(The case does not have a {keyword} keyword.
  Please add a {keyword} keyword in the RUNSPEC section specifying at least {schedVal} {entity} in item {item})",
                                             "keyword"_a = keyword,
                                             "schedVal"_a = schedVal,
                                             "entity"_a = entity,
                                             "item"_a = item);

        ctxt.handleError(ctxtKey, message_fmt, {}, guard);
    }

    void reportError(const Opm::KeywordLocation& location,
                     const std::size_t           maxVal,
                     const std::size_t           schedVal,
                     const std::size_t           item,
                     std::string_view            entity,
                     const std::string&          ctxtKey,
                     const Opm::ParseContext&    ctxt,
                     Opm::ErrorGuard&            guard)
    {
        using namespace fmt::literals;

        const auto* pl = (maxVal != 1) ? "are" : "is";

        // Note: Number of leading blanks on lines 2-4 affects the formatted output.
        const auto message_fmt = fmt::format(R"(Problem with keyword {{keyword}}
  In {{file}} line {{line}}
  The case has {schedVal} {entity}, but at most {maxVal} {pl} allowed in {{keyword}}.
  Please increase item {item} in {{keyword}} to at least {schedVal})",
                                             "schedVal"_a = schedVal,
                                             "maxVal"_a = maxVal,
                                             "entity"_a = entity,
                                             "item"_a = item,
                                             "pl"_a = pl);

        ctxt.handleError(ctxtKey, message_fmt, location, guard);
    }

    void reportError(const Opm::KeywordLocation& location,
                     const std::size_t           maxVal,
                     const std::size_t           schedVal,
                     const std::size_t           item,
                     std::string_view            hostEntity,
                     std::string_view            entity,
                     const std::string&          ctxtKey,
                     const Opm::ParseContext&    ctxt,
                     Opm::ErrorGuard&            guard)
    {
        using namespace fmt::literals;

        const auto* pl = (maxVal != 1) ? "are" : "is";

        // Note: Number of leading blanks on lines 2-4 affects the formatted output.
        const auto message_fmt = fmt::format(R"(Problem with keyword {{keyword}}
  In {{file}} line {{line}}
  The case has a {hostEntity} with {schedVal} {entity}, but at most {maxVal} {pl} allowed in {{keyword}}.
  Please increase item {item} of {{keyword}} to at least {schedVal})",
                                             "schedVal"_a = schedVal,
                                             "maxVal"_a = maxVal,
                                             "hostEntity"_a = hostEntity,
                                             "entity"_a = entity,
                                             "item"_a = item,
                                             "pl"_a = pl);

        ctxt.handleError(ctxtKey, message_fmt, location, guard);
    }

    namespace WellDims {

        void checkNumWells(const Opm::Welldims&     wdims,
                           const Opm::Schedule&     sched,
                           const Opm::ParseContext& ctxt,
                           Opm::ErrorGuard&         guard)
        {
            const auto nWells = sched.numWells();

            if (nWells <= std::size_t(wdims.maxWellsInField())) {
                return;
            }

            const auto item = 1; // MAXWELLS = WELLDIMS(1)
            const auto* entity = (nWells == 1) ? "well" : "wells";

            if (const auto& location = wdims.location(); location.has_value()) {
                reportError(*location, wdims.maxWellsInField(), nWells, item, entity,
                            Opm::ParseContext::RUNSPEC_NUMWELLS_TOO_LARGE, ctxt, guard);
            }
            else {
                reportError("WELLDIMS", nWells, item, entity,
                            Opm::ParseContext::RUNSPEC_NUMWELLS_TOO_LARGE, ctxt, guard);
            }
        }

        void checkConnPerWell(const Opm::Welldims&     wdims,
                              const Opm::Schedule&     sched,
                              const Opm::ParseContext& ctxt,
                              Opm::ErrorGuard&         guard)
        {
            auto nconn = std::size_t{0};
            for (const auto& well_name : sched.wellNames()) {
                const auto& well = sched.getWellatEnd(well_name);
                nconn = std::max(nconn, well.getConnections().size());
            }

            if (nconn <= static_cast<decltype(nconn)>(wdims.maxConnPerWell())) {
                return;
            }

            const auto item = 2; // MAXCONN = WELLDIMS(2)
            const auto* entity = (nconn == 1) ? "connection" : "connections";
            const auto* hostEntity = "well";

            if (const auto& location = wdims.location(); location.has_value()) {
                reportError(*location, wdims.maxConnPerWell(), nconn, item, hostEntity, entity,
                            Opm::ParseContext::RUNSPEC_CONNS_PER_WELL_TOO_LARGE, ctxt, guard);
            }
            else {
                reportError("WELLDIMS", nconn, item, entity,
                            Opm::ParseContext::RUNSPEC_CONNS_PER_WELL_TOO_LARGE, ctxt, guard);
            }
        }

        void checkNumGroups(const Opm::Welldims&     wdims,
                            const Opm::Schedule&     sched,
                            const Opm::ParseContext& ctxt,
                            Opm::ErrorGuard&         guard)
        {
            const auto nGroups = sched.back().groups.size();

            // Note: "1 +" to account for FIELD group being in 'sched.numGroups()'
            //   but excluded from WELLDIMS(3).
            if (nGroups <= 1U + wdims.maxGroupsInField()) {
                return;
            }

            const auto item = 3; // MAXGROUPS = WELLDIMS(3)
            const auto* entity = (nGroups == 1 + 1)
                ? "non-FIELD group"
                : "non-FIELD groups";

            if (const auto& location = wdims.location(); location.has_value()) {
                reportError(*location, wdims.maxGroupsInField(), nGroups - 1, item, entity,
                            Opm::ParseContext::RUNSPEC_NUMGROUPS_TOO_LARGE, ctxt, guard);
            }
            else {
                reportError("WELLDIMS", nGroups - 1, item, entity,
                            Opm::ParseContext::RUNSPEC_NUMGROUPS_TOO_LARGE, ctxt, guard);
            }
        }

        void checkGroupSize(const Opm::Welldims&     wdims,
                            const Opm::Schedule&     sched,
                            const Opm::ParseContext& ctxt,
                            Opm::ErrorGuard&         guard)
        {
            const auto numSteps = sched.size() - 1;

            auto size = std::size_t{0};
            for (auto step = 0*numSteps; step < numSteps; ++step) {
                const auto nwgmax = maxGroupSize(sched, step);

                size = std::max(size, static_cast<std::size_t>(nwgmax));
            }

            if (size <= static_cast<decltype(size)>(wdims.maxWellsPerGroup())) {
                return;
            }

            const auto item = 4; // MAX_GROUPSIZE = WELLDIMS(4)
            const auto* entity = (size == 1) ? "child" : "children";
            const auto* hostEntity = "group";

            if (const auto& location = wdims.location(); location.has_value()) {
                reportError(*location, wdims.maxWellsPerGroup(), size, item, hostEntity, entity,
                            Opm::ParseContext::RUNSPEC_GROUPSIZE_TOO_LARGE, ctxt, guard);

            }
            else {
                reportError("WELLDIMS", size, item, entity,
                            Opm::ParseContext::RUNSPEC_GROUPSIZE_TOO_LARGE, ctxt, guard);
            }
        }
    } // WellDims

    void consistentWellDims(const Opm::Welldims&     wdims,
                            const Opm::Schedule&     sched,
                            const Opm::ParseContext& ctxt,
                            Opm::ErrorGuard&         guard)
    {
        WellDims::checkNumWells   (wdims, sched, ctxt, guard);
        WellDims::checkConnPerWell(wdims, sched, ctxt, guard);
        WellDims::checkNumGroups  (wdims, sched, ctxt, guard);
        WellDims::checkGroupSize  (wdims, sched, ctxt, guard);
    }
} // Anonymous

void
Opm::checkConsistentArrayDimensions(const EclipseState& es,
                                    const Schedule&     sched,
                                    const ParseContext& ctxt,
                                    ErrorGuard&         guard)
{
    consistentWellDims(es.runspec().wellDimensions(), sched, ctxt, guard);
}




int
Opm::maxGroupSize(const Opm::Schedule& sched,
                  const std::size_t    step)
{
    int nwgmax = 0;

    for (const auto& gnm : sched.groupNames(step)) {
        const auto& grp = sched.getGroup(gnm, step);
        const auto  gsz = grp.wellgroup()
            ? grp.numWells() : grp.groups().size();

        nwgmax = std::max(nwgmax, static_cast<int>(gsz));
    }

    return nwgmax;
}
