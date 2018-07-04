/*
  Copyright 2018 Statoil ASA

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

#include <opm/output/eclipse/AggregateWellData.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/output/eclipse/SummaryState.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>

namespace VI = Opm::RestartIO::Helpers::VectorItems;

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateWellData
// ---------------------------------------------------------------------

namespace {
    std::size_t numWells(const std::vector<int>& inteHead)
    {
        return inteHead[VI::intehead::NWELLS];
    }

    int maxNumGroups(const std::vector<int>& inteHead)
    {
        return inteHead[VI::intehead::NWGMAX];
    }

    std::string trim(const std::string& s)
    {
        const auto b = s.find_first_not_of(" \t");
        if (b == std::string::npos) {
            // No blanks.  Return unchanged.
            return s;
        }

        auto e = s.find_last_not_of(" \t");
        if (e == std::string::npos) {
            // No trailing blanks.  Return [b, end)
            return s.substr(b, std::string::npos);
        }

        // Remove leading/trailing blanks.
        return s.substr(b, e - b + 1);
    }

    std::vector<std::string>
    groupNames(const std::vector<const Opm::Group*>& groups)
    {
        auto gnms = std::vector<std::string>{};
        gnms.reserve(groups.size());

        for (const auto* group : groups) {
            gnms.push_back(trim(group->name()));
        }

        return gnms;
    }

    template <typename WellOp>
    void wellLoop(const std::vector<const Opm::Well*>& wells,
                  WellOp&&                             wellOp)
    {
        for (auto nWell = wells.size(), wellID = 0*nWell;
             wellID < nWell; ++wellID)
        {
            const auto* well = wells[wellID];

            if (well == nullptr) { continue; }

            wellOp(*well, wellID);
        }
    }

    namespace IWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            return inteHead[VI::intehead::NIWELZ];
        }

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

            return WV {
                WV::NumWindows{ numWells(inteHead) },
                WV::WindowSize{ entriesPerWell(inteHead) }
            };
        }

        int groupIndex(const std::string&              grpName,
                       const std::vector<std::string>& groupNames,
                       const int                       maxGroups)
        {
            if (grpName == "FIELD") {
                // Not really supposed to happen since wells are
                // not supposed to be parented dirctly to FIELD.
                return maxGroups + 1;
            }

            auto b = std::begin(groupNames);
            auto e = std::end  (groupNames);
            auto i = std::find(b, e, grpName);

            if (i == e) {
                // Not really supposed to happen since wells are
                // not supposed to be parented dirctly to FIELD.
                return maxGroups + 1;
            }

            // One-based indexing.
            return std::distance(b, i) + 1;
        }

        int wellType(const Opm::Well&  well,
                     const std::size_t sim_step)
        {
            if (well.isProducer(sim_step)) {
                return 1;      // Producer flag
            }

            using IType = ::Opm::WellInjector::TypeEnum;

            const auto itype = well
                .getInjectionProperties(sim_step).injectorType;

            switch (itype) {
            case IType::OIL:   return 2; // Oil Injector
            case IType::WATER: return 3; // Water Injector
            case IType::GAS:   return 4; // Gas Injector
            default:           return 0; // Undefined
            }
        }

        int wellVFPTab(const Opm::Well&  well,
                       const std::size_t sim_step)
        {
            if (well.isInjector(sim_step)) {
                return well.getInjectionProperties(sim_step).VFPTableNumber;
            }

            return well.getProductionProperties(sim_step).VFPTableNumber;
        }

        int ctrlMode(const Opm::Well&  well,
                     const std::size_t sim_step)
        {
            {
                const auto stat = well.getStatus(sim_step);

                using WStat = ::Opm::WellCommon::StatusEnum;

                if ((stat == WStat::SHUT) || (stat == WStat::STOP)) {
                    return 0;
                }
            }

            if (well.isInjector(sim_step)) {
                const auto& prop = well
                    .getInjectionProperties(sim_step);

                const auto wmctl = prop.controlMode;
                const auto wtype = prop.injectorType;

                using CMode = ::Opm::WellInjector::ControlModeEnum;
                using WType = ::Opm::WellInjector::TypeEnum;

                switch (wmctl) {
                case CMode::RATE: {
                    switch (wtype) {
                    case WType::OIL:   return 1; // ORAT
                    case WType::WATER: return 2; // WRAT
                    case WType::GAS:   return 3; // GRAT
                    case WType::MULTI: return 0; // MULTI (value not known)
                    }
                }
                    break;

                case CMode::RESV: return 5; // RESV
                case CMode::THP:  return 6;
                case CMode::BHP:  return 7;
                case CMode::GRUP: return -1;

                default:
                    return 0;
                }
            }
            else if (well.isProducer(sim_step)) {
                const auto& prop = well
                    .getProductionProperties(sim_step);

                using CMode = ::Opm::WellProducer::ControlModeEnum;

                switch (prop.controlMode) {
                case CMode::ORAT: return 1;
                case CMode::WRAT: return 2;
                case CMode::GRAT: return 3;
                case CMode::LRAT: return 4;
                case CMode::RESV: return 5;
                case CMode::THP:  return 6;
                case CMode::BHP:  return 7;
                case CMode::CRAT: return 9;
                case CMode::GRUP: return -1;

                default: return 0;
                }
            }

            return 0;
        }

        int compOrder(const Opm::Well& well)
        {
            using WCO = ::Opm::WellCompletion::CompletionOrderEnum;

            switch (well.getWellConnectionOrdering()) {
            case WCO::TRACK: return 0;
            case WCO::DEPTH: return 1; // Not really supported in Flow
            case WCO::INPUT: return 2;
            }

            return 0;
        }

        template <class IWellArray>
        void staticContrib(const Opm::Well&                well,
                           const std::size_t               msWellID,
                           const std::vector<std::string>& groupNames,
                           const int                       maxGroups,
                           const std::size_t               sim_step,
                           IWellArray&                     iWell)
        {
            iWell[1 - 1] = well.getHeadI(sim_step) + 1;
            iWell[2 - 1] = well.getHeadJ(sim_step) + 1;

            // Connections
            {
                const auto& conn = well.getConnections(sim_step);

                // IWEL(5) = Number of active cells connected to well.
                iWell[5 - 1] = static_cast<int>(conn.size());

                // IWEL(3) = Layer ID of top connection.
                iWell[3 - 1] = (iWell[5 - 1] == 0)
                    ? 0 : conn.get(0).getK() + 1;

                // IWEL(4) = Layer ID of last connection.
                iWell[4 - 1] = (iWell[5 - 1] == 0)
                    ? 0 : conn.get(conn.size() - 1).getK() + 1;
            }

            iWell[ 6 - 1] =
                groupIndex(trim(well.getGroupName(sim_step)),
                           groupNames, maxGroups);

            iWell[ 7 - 1] = wellType  (well, sim_step);
            iWell[ 8 - 1] = ctrlMode  (well, sim_step);
            iWell[12 - 1] = wellVFPTab(well, sim_step);

            // The following items aren't fully characterised yet, but
            // needed for restart of M2.  Will need further refinement.
            iWell[18 - 1] = -100;
            iWell[25 - 1] = -  1;
            iWell[32 - 1] =    7;
            iWell[48 - 1] = -  1;

            iWell[50 - 1] = iWell[8 - 1];  // Copy of ctrl mode.

            // Multi-segmented well information
            iWell[71 - 1] = 0;  // MS Well ID (0 or 1..#MS wells)
            iWell[72 - 1] = 0;  // Number of well segments
            if (well.isMultiSegment(sim_step)) {
                iWell[71 - 1] = static_cast<int>(msWellID);
                iWell[72 - 1] =
                    well.getWellSegments(sim_step).size();
            }

            iWell[99 - 1] = compOrder(well);
        }

        template <class IWellArray>
        void dynamicContribShut(IWellArray& iWell)
        {
            iWell[ 9 - 1] = -1000;
            iWell[11 - 1] = -1000;
        }

        template <class IWellArray>
        void dynamicContribOpen(const Opm::data::Well& xw,
                                IWellArray&            iWell)
        {
            const auto any_flowing_conn =
                std::any_of(std::begin(xw.connections),
                            std::end  (xw.connections),
                    [](const Opm::data::Connection& c)
                {
                    return c.rates.any();
                });

            iWell[ 9 - 1] = any_flowing_conn ? iWell[8 - 1] : -1;
            iWell[11 - 1] = 1;
        }
    } // IWell

    namespace SWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            assert ((inteHead[VI::intehead::NSWELZ] > 121) &&
                    "SWEL must allocate at least 122 elements per well");

            return inteHead[VI::intehead::NSWELZ];
        }

        float datumDepth(const Opm::Well&  well,
                         const std::size_t sim_step)
        {
            if (well.isMultiSegment(sim_step)) {
                // Datum depth for multi-segment wells is
                // depth of top-most segment.
                return well.getWellSegments(sim_step)
                    .depthTopSegment();
            }

            // Not a multi-segment well--i.e., this is a regular
            // well.  Use well's reference depth.
            return well.getRefDepth(sim_step);
        }

        Opm::RestartIO::Helpers::WindowedArray<float>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<float>;

            return WV {
                WV::NumWindows{ numWells(inteHead) },
                WV::WindowSize{ entriesPerWell(inteHead) }
            };
        }

        std::vector<float> defaultSWell()
        {
            const auto dflt  = -1.0e+20f;
            const auto infty =  1.0e+20f;
            const auto zero  =  0.0f;
            const auto one   =  1.0f;
            const auto half  =  0.5f;

            // Initial data by Statoil ASA.
            return { // 122 Items (0..121)
                // 0     1      2      3      4      5
                infty, infty, infty, infty, infty, infty,    //   0..  5  ( 0)
                one  , zero , zero , zero , zero , 1.0e-05f, //   6.. 11  ( 1)
                zero , zero , infty, infty, zero , dflt ,    //  12.. 17  ( 2)
                infty, infty, infty, infty, infty, zero ,    //  18.. 23  ( 3)
                one  , zero , zero , zero , zero , zero ,    //  24.. 29  ( 4)
                zero , one  , zero , infty, zero , zero ,    //  30.. 35  ( 5)
                zero , zero , zero , zero , zero , zero ,    //  36.. 41  ( 6)
                zero , zero , zero , zero , zero , zero ,    //  42.. 47  ( 7)
                zero , zero , zero , zero , zero , zero ,    //  48.. 53  ( 8)
                infty, zero , zero , zero , zero , zero ,    //  54.. 59  ( 9)
                zero , zero , zero , zero , zero , zero ,    //  60.. 65  (10)
                zero , zero , zero , zero , zero , zero ,    //  66.. 71  (11)
                zero , zero , zero , zero , zero , zero ,    //  72.. 77  (12)
                zero , infty, infty, zero , zero , one  ,    //  78.. 83  (13)
                one  , one  , zero , infty, zero , infty,    //  84.. 89  (14)
                one  , dflt , one  , zero , zero , zero ,    //  90.. 95  (15)
                zero , zero , zero , zero , zero , zero ,    //  96..101  (16)
                zero , zero , zero , zero , zero , zero ,    // 102..107  (17)
                zero , zero , half , one  , zero , zero ,    // 108..113  (18)
                zero , zero , zero , zero , zero , infty,    // 114..119  (19)
                zero , one  ,                                // 120..121  (20)
            };
        }

        template <class SWellArray>
        void assignDefaultSWell(SWellArray& sWell)
        {
            const auto& init = defaultSWell();

            const auto sz = static_cast<
                decltype(init.size())>(sWell.size());

            auto b = std::begin(init);
            auto e = b + std::min(init.size(), sz);

            std::copy(b, e, std::begin(sWell));
        }

        template <class SWellArray>
        void staticContrib(const Opm::Well&       well,
                           const Opm::UnitSystem& units,
                           const std::size_t      sim_step,
                           SWellArray&            sWell)
        {
            using M = ::Opm::UnitSystem::measure;

            auto swprop = [&units](const M u, const double x) -> float
            {
                return static_cast<float>(units.from_si(u, x));
            };

            assignDefaultSWell(sWell);

            if (well.isProducer(sim_step)) {
                const auto& pp = well.getProductionProperties(sim_step);

                using PP = ::Opm::WellProducer::ControlModeEnum;

                if (pp.hasProductionControl(PP::ORAT)) {
                    sWell[1 - 1] = swprop(M::liquid_surface_rate, pp.OilRate);
                }

                if (pp.hasProductionControl(PP::WRAT)) {
                    sWell[2 - 1] = swprop(M::liquid_surface_rate, pp.WaterRate);
                }

                if (pp.hasProductionControl(PP::GRAT)) {
                    sWell[3 - 1] = swprop(M::gas_surface_rate, pp.GasRate);
                }

                if (pp.hasProductionControl(PP::LRAT)) {
                    sWell[4 - 1] = swprop(M::liquid_surface_rate, pp.LiquidRate);
                }

                if (pp.hasProductionControl(PP::RESV)) {
                    sWell[5 - 1] = swprop(M::rate, pp.ResVRate);
                }

                if (pp.hasProductionControl(PP::THP)) {
                    sWell[6 - 1] = swprop(M::pressure, pp.THPLimit);
                }

                if (pp.hasProductionControl(PP::BHP)) {
                    sWell[7 - 1] = swprop(M::pressure, pp.BHPLimit);
                }
            }
            else if (well.isInjector(sim_step)) {
                const auto& ip = well.getInjectionProperties(sim_step);

                using IP = ::Opm::WellInjector::ControlModeEnum;

                if (ip.hasInjectionControl(IP::THP)) {
                    sWell[6 - 1] = swprop(M::pressure, ip.THPLimit);
                }

                if (ip.hasInjectionControl(IP::BHP)) {
                    sWell[7 - 1] = swprop(M::pressure, ip.BHPLimit);
                }
            }

            sWell[10 - 1] = swprop(M::length, datumDepth(well, sim_step));
        }
    } // SWell

    namespace XWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            assert ((inteHead[VI::intehead::NXWELZ] > 123) &&
                    "XWEL must allocate at least 124 elements per well");

            return inteHead[VI::intehead::NXWELZ];
        }

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<double>;

            return WV {
                WV::NumWindows{ numWells(inteHead) },
                WV::WindowSize{ entriesPerWell(inteHead) }
            };
        }

        template <class XWellArray>
        void staticContrib(const ::Opm::Well&     well,
                           const Opm::UnitSystem& units,
                           const std::size_t      sim_step,
                           XWellArray&            xWell)
        {
            using M = ::Opm::UnitSystem::measure;

            // xWell[41] = BHP target.

            xWell[41] = well.isInjector(sim_step)
                ? well.getInjectionProperties (sim_step).BHPLimit
                : well.getProductionProperties(sim_step).BHPLimit;

            xWell[41] = units.from_si(M::pressure, xWell[41]);
        }

        template <class XWellArray>
        void assignProducer(const std::string&         well,
                            const ::Opm::SummaryState& smry,
                            XWellArray&                xWell)
        {
            auto get = [&smry, &well](const std::string& vector)
            {
                return smry.get(vector + ':' + well);
            };

            xWell[0] = get("WOPR");
            xWell[1] = get("WWPR");
            xWell[2] = get("WGPR");
            xWell[3] = xWell[0] + xWell[1]; // LPR
            xWell[4] = get("WVPR");

            xWell[6] = get("WBHP");
            xWell[7] = get("WWCT");
            xWell[8] = get("WGOR");

            xWell[18] = get("WOPT");
            xWell[19] = get("WWPT");
            xWell[20] = get("WGPT");
            xWell[21] = get("WVPT");

            xWell[36] = xWell[1]; // Copy of WWPR
            xWell[37] = xWell[2]; // Copy of WGPR
        }

        template <class XWellArray>
        void assignWaterInjector(const std::string&         well,
                                 const ::Opm::SummaryState& smry,
                                 XWellArray&                xWell)
        {
            auto get = [&smry, &well](const std::string& vector)
            {
                return smry.get(vector + ':' + well);
            };

            // Rates reported as negative, cumulative totals as positive.
            xWell[1] = -get("WWIR");
            xWell[3] = xWell[1];  // Copy of WWIR

            xWell[6] = get("WBHP");

            xWell[23] = get("WWIT");

            xWell[36] = xWell[1];  // Copy of WWIR
            xWell[81] = xWell[23]; // Copy of WWIT

            xWell[122] = -get("WWVIR");
        }

        template <class XWellArray>
        void assignGasInjector(const std::string&         well,
                               const ::Opm::SummaryState& smry,
                               XWellArray&                xWell)
        {
            auto get = [&smry, &well](const std::string& vector)
            {
                return smry.get(vector + ':' + well);
            };

            // Rates reported as negative, cumulative totals as positive.
            xWell[2] = -get("WGIR");
            xWell[4] = -get("WGVIR");

            xWell[6] = get("WBHP");

            xWell[24] = get("WGIT");

            xWell[34] = xWell[4] / xWell[2]; // Bg

            xWell[37] = xWell[2]; // Copy of WGIR

            xWell[82] = xWell[24]; // Copy of WGIT

            xWell[123] = xWell[4]; // Copy of WGVIR
        }

        template <class XWellArray>
        void dynamicContrib(const ::Opm::Well&         well,
                            const ::Opm::SummaryState& smry,
                            const std::size_t          sim_step,
                            XWellArray&                xWell)
        {
            if (well.isProducer(sim_step)) {
                assignProducer(well.name(), smry, xWell);
            }
            else if (well.isInjector(sim_step)) {
                using IType = ::Opm::WellInjector::TypeEnum;

                const auto itype = well
                    .getInjectionProperties(sim_step).injectorType;

                switch (itype) {
                case IType::OIL:
                    // Do nothing.
                    break;

                case IType::WATER:
                    assignWaterInjector(well.name(), smry, xWell);
                    break;

                case IType::GAS:
                    assignGasInjector(well.name(), smry, xWell);
                    break;

                case IType::MULTI:
                    assignWaterInjector(well.name(), smry, xWell);
                    assignGasInjector  (well.name(), smry, xWell);
                    break;
                }
            }
        }
    } // XWell

    namespace ZWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            assert ((inteHead[VI::intehead::NZWELZ] > 1) &&
                    "ZWEL must allocate at least 1 element per well");

            return inteHead[VI::intehead::NZWELZ];
        }

        Opm::RestartIO::Helpers::WindowedArray<
            Opm::RestartIO::Helpers::CharArrayNullTerm<8>
        >
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<
                Opm::RestartIO::Helpers::CharArrayNullTerm<8>
            >;

            return WV {
                WV::NumWindows{ numWells(inteHead) },
                WV::WindowSize{ entriesPerWell(inteHead) }
            };
        }

        template <class ZWellArray>
        void staticContrib(const Opm::Well& well, ZWellArray& zWell)
        {
            zWell[1 - 1] = well.name();
        }
    } // ZWell
} // Anonymous

// =====================================================================

Opm::RestartIO::Helpers::AggregateWellData::
AggregateWellData(const std::vector<int>& inteHead)
    : iWell_ (IWell::allocate(inteHead))
    , sWell_ (SWell::allocate(inteHead))
    , xWell_ (XWell::allocate(inteHead))
    , zWell_ (ZWell::allocate(inteHead))
    , nWGMax_(maxNumGroups(inteHead))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateWellData::
captureDeclaredWellData(const Schedule&   sched,
                        const UnitSystem& units,
                        const std::size_t sim_step)
{
    const auto& wells = sched.getWells(sim_step);

    // Static contributions to IWEL array.
    {
        const auto grpNames = groupNames(sched.getGroups());
        auto msWellID       = std::size_t{0};

        wellLoop(wells, [&grpNames, &msWellID, sim_step, this]
            (const Well& well, const std::size_t wellID) -> void
        {
            msWellID += well.isMultiSegment(sim_step);  // 1-based index.
            auto iw   = this->iWell_[wellID];

            IWell::staticContrib(well, msWellID, grpNames,
                                 this->nWGMax_, sim_step, iw);
        });
    }

    // Static contributions to SWEL array.
    wellLoop(wells, [&units, sim_step, this]
        (const Well& well, const std::size_t wellID) -> void
    {
        auto sw = this->sWell_[wellID];

        SWell::staticContrib(well, units, sim_step, sw);
    });

    // Static contributions to XWEL array.
    wellLoop(wells, [&units, sim_step, this]
        (const Well& well, const std::size_t wellID) -> void
    {
        auto xw = this->xWell_[wellID];

        XWell::staticContrib(well, units, sim_step, xw);
    });

    // Static contributions to ZWEL array.
    wellLoop(wells,
        [this](const Well& well, const std::size_t wellID) -> void
    {
        auto zw = this->zWell_[wellID];

        ZWell::staticContrib(well, zw);
    });
}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateWellData::
captureDynamicWellData(const Schedule&             sched,
                       const std::size_t           sim_step,
                       const Opm::data::WellRates& xw,
                       const ::Opm::SummaryState&  smry)
{
    const auto& wells = sched.getWells(sim_step);

    // Dynamic contributions to IWEL array.
    wellLoop(wells, [this, &xw]
        (const Well& well, const std::size_t wellID) -> void
    {
        auto iWell = this->iWell_[wellID];

        auto i = xw.find(well.name());
        if ((i == std::end(xw)) || !i->second.flowing()) {
            IWell::dynamicContribShut(iWell);
        }
        else {
            IWell::dynamicContribOpen(i->second, iWell);
        }
    });

    // Dynamic contributions to XWEL array.
    wellLoop(wells, [this, sim_step, &smry]
        (const Well& well, const std::size_t wellID) -> void
    {
        auto xw = this->xWell_[wellID];

        XWell::dynamicContrib(well, smry, sim_step, xw);
    });
}
