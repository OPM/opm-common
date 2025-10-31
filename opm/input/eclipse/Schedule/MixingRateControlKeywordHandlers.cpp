/*
  Copyright 2020 Statoil ASA.

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

#include "MixingRateControlKeywordHandlers.hpp"

#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/ScheduleStatic.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>

#include "HandlerContext.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

namespace {

void handleDRSDT(HandlerContext& handlerContext)
{
    const std::size_t numPvtRegions = handlerContext.static_schedule()
        .m_runspec.tabdims().getNumPVTTables();

    std::vector<double> maximums(numPvtRegions);
    std::vector<std::string> options(numPvtRegions);

    for (const auto& record : handlerContext.keyword) {
        const auto max = record
            .getItem<ParserKeywords::DRSDT::DRSDT_MAX>()
            .getSIDouble(0);

        const auto option = record
            .getItem<ParserKeywords::DRSDT::OPTION>()
            .get<std::string>(0);

        std::fill(maximums.begin(), maximums.end(), max);
        std::fill(options.begin(), options.end(), option);

        auto ovp = handlerContext.state().oilvap();
        OilVaporizationProperties::updateDRSDT(ovp, maximums, options);

        handlerContext.state().oilvap.update(std::move(ovp));
    }
}

void handleDRSDTCON(HandlerContext& handlerContext)
{
    const std::size_t numPvtRegions = handlerContext.static_schedule()
        .m_runspec.tabdims().getNumPVTTables();

    std::vector<double> maximums(numPvtRegions);
    std::vector<std::string> options(numPvtRegions);
    std::vector<double> psis(numPvtRegions);
    std::vector<double> omegas(numPvtRegions);

    std::size_t pvtRegionIdx = 0;
    for (const auto& record : handlerContext.keyword) {
        const auto max = record
            .getItem<ParserKeywords::DRSDTCON::DRSDT_MAX>()
            .getSIDouble(0);

        const auto omega = record
            .getItem<ParserKeywords::DRSDTCON::OMEGA>()
            .getSIDouble(0);

        const auto psi = record
            .getItem<ParserKeywords::DRSDTCON::PSI>()
            .getSIDouble(0);

        const auto option = record
            .getItem<ParserKeywords::DRSDTCON::OPTION>()
            .get<std::string>(0);

        maximums[pvtRegionIdx] = max;
        options [pvtRegionIdx] = option;
        psis    [pvtRegionIdx] = psi;
        omegas  [pvtRegionIdx] = omega;

        ++pvtRegionIdx;
    }

    auto ovp = handlerContext.state().oilvap();
    OilVaporizationProperties::updateDRSDTCON(ovp, maximums, options, psis, omegas);

    handlerContext.state().oilvap.update(std::move(ovp));
}

void handleDRSDTR(HandlerContext& handlerContext)
{
    const std::size_t numPvtRegions = handlerContext.static_schedule()
        .m_runspec.tabdims().getNumPVTTables();

    std::vector<double> maximums(numPvtRegions);
    std::vector<std::string> options(numPvtRegions);

    std::size_t pvtRegionIdx = 0;
    for (const auto& record : handlerContext.keyword) {
        const auto max = record
            .getItem<ParserKeywords::DRSDTR::DRSDT_MAX>()
            .getSIDouble(0);

        const auto& option = record
            .getItem<ParserKeywords::DRSDTR::OPTION>()
            .get<std::string>(0);

        maximums[pvtRegionIdx] = max;
        options[pvtRegionIdx] = option;

        ++pvtRegionIdx;
    }

    auto ovp = handlerContext.state().oilvap();
    OilVaporizationProperties::updateDRSDT(ovp, maximums, options);

    handlerContext.state().oilvap.update(std::move(ovp));
}

void handleDRVDT(HandlerContext& handlerContext)
{
    const std::size_t numPvtRegions = handlerContext.static_schedule()
        .m_runspec.tabdims().getNumPVTTables();

    std::vector<double> maximums(numPvtRegions);
    for (const auto& record : handlerContext.keyword) {
        const auto max = record
            .getItem<ParserKeywords::DRVDTR::DRVDT_MAX>()
            .getSIDouble(0);

        std::fill(maximums.begin(), maximums.end(), max);

        auto ovp = handlerContext.state().oilvap();
        OilVaporizationProperties::updateDRVDT(ovp, maximums);

        handlerContext.state().oilvap.update(std::move(ovp));
    }
}

void handleDRVDTR(HandlerContext& handlerContext)
{
    const std::size_t numPvtRegions = handlerContext.static_schedule()
        .m_runspec.tabdims().getNumPVTTables();

    std::vector<double> maximums(numPvtRegions);

    std::size_t pvtRegionIdx = 0;
    for (const auto& record : handlerContext.keyword) {
        maximums[pvtRegionIdx] = record
            .getItem<ParserKeywords::DRVDTR::DRVDT_MAX>()
            .getSIDouble(0);

        ++pvtRegionIdx;
    }

    auto ovp = handlerContext.state().oilvap();
    OilVaporizationProperties::updateDRVDT(ovp, maximums);

    handlerContext.state().oilvap.update(std::move(ovp));
}

void handleVAPPARS(HandlerContext& handlerContext)
{
    using Kw = ParserKeywords::VAPPARS;

    for (const auto& record : handlerContext.keyword) {
        const auto vap1 = record
            .getItem<Kw::OIL_VAP_PROPENSITY>()
            .get<double>(0);

        const auto vap2 = record
            .getItem<Kw::OIL_DENSITY_PROPENSITY>()
            .get<double>(0);

        auto ovp = handlerContext.state().oilvap();
        OilVaporizationProperties::updateVAPPARS(ovp, vap1, vap2);

        handlerContext.state().oilvap.update(std::move(ovp));
    }
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getMixingRateControlHandlers()
{
    return {
        { "DRSDT"   , &handleDRSDT    },
        { "DRSDTCON", &handleDRSDTCON },
        { "DRSDTR"  , &handleDRSDTR   },
        { "DRVDT"   , &handleDRVDT    },
        { "DRVDTR"  , &handleDRVDTR   },
        { "VAPPARS" , &handleVAPPARS  },
    };
}

}
