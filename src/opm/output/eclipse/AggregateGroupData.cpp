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

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateWellData
// ---------------------------------------------------------------------

namespace {
    std::size_t numWells(const std::vector<int>& inteHead)
    {
        // INTEHEAD(17) = NWELLS
        return inteHead[17 - 1];
    }

    int maxNumGroups(const std::vector<int>& inteHead)
    {
        return inteHead[20 - 1];
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
        auto wellID = std::size_t{0};
        for (const auto* well : wells) {
            wellID += 1;

            if (well == nullptr) { continue; }

            wellOp(*well, wellID - 1);
        }
    }

    namespace IWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            // INTEHEAD(25) = NIWELZ
            return inteHead[25 - 1];
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
                // Not really supposed to happen...
                return maxGroups + 1;
            }

            auto b = std::begin(groupNames);
            auto e = std::end  (groupNames);
            auto i = std::find(b, e, grpName);

            if (i == e) {
                // Not really supposed to happen...
                return maxGroups + 1;
            }

            // One-based indexing.
            return std::distance(b, i) + 1;
        }

        int wellType(const Opm::Well&  well,
                     const std::size_t rptStep)
        {
            if (well.isProducer(rptStep)) {
                return 1;      // Producer flag
            }

            using IType = ::Opm::WellInjector::TypeEnum;

            const auto itype = well
                .getInjectionProperties(rptStep).injectorType;

            switch (itype) {
            case IType::OIL:   return 2; // Oil Injector
            case IType::WATER: return 3; // Water Injector
            case IType::GAS:   return 4; // Gas Injector
            default:           return 0; // Undefined
            }
        }

        int wellVFPTab(const Opm::Well&  well,
                       const std::size_t rptStep)
        {
            if (well.isInjector(rptStep)) {
                return well.getInjectionProperties(rptStep).VFPTableNumber;
            }

            return well.getProductionProperties(rptStep).VFPTableNumber;
        }

#if 0
        int ctrlMode(const Opm::Well&  well,
                     const std::size_t rptStep)
        {
            {
                const auto stat = well.getStatus(rptStep);

                using WStat = ::Opm::WellCommon::StatusEnum;

                if ((stat == WStat::SHUT) || (stat == WStat::STOP)) {
                    return 0;
                }
            }

            if (well.isInjector(rptStep)) {
                const auto& prop = well
                    .getInjectionProperties(rptStep);

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
            else if (well.isProducer(rptStep)) {
                const auto& prop = well
                    .getProductionProperties(rptStep);

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
        }
#endif

        int compOrder(const Opm::Well& well)
        {
            using WCO = ::Opm::WellCompletion::CompletionOrderEnum;

            switch (well.getWellCompletionOrdering()) {
            case WCO::TRACK: return 0;
            case WCO::DEPTH: return 1; // Not really supported in Flow
            case WCO::INPUT: return 2;
            }

            return 0;
        }

        template <class IWellArray>
        void staticContrib(const Opm::Well&                well,
                           const std::vector<std::string>& groupNames,
                           const int                       maxGroups,
                           const std::size_t               rptStep,
                           IWellArray&                     iWell)
        {
            iWell[1 - 1] = well.getHeadI(rptStep) + 1;
            iWell[2 - 1] = well.getHeadJ(rptStep) + 1;

            // Completions
            {
                const auto& cmpl = well.getCompletions(rptStep);

                iWell[5 - 1] = static_cast<int>(cmpl.size());
                iWell[3 - 1] = (iWell[5 - 1] == 0)
                    ? 0 : cmpl.get(0).getK() + 1;
            }

            iWell[ 6 - 1] =
                groupIndex(trim(well.getGroupName(rptStep)),
                           groupNames, maxGroups);

            iWell[ 7 - 1] = wellType  (well, rptStep);
            iWell[12 - 1] = wellVFPTab(well, rptStep);

            // The following items aren't fully characterised yet, but
            // needed for restart of M2.  Will need further refinement.
            iWell[17 - 1] = -100;
            iWell[25 - 1] = -  1;
            iWell[32 - 1] =    7;
            iWell[48 - 1] = -  1;

            // Multi-segmented well information (not complete)
            iWell[71 - 1] = 0;  // ID of Segmented Well
            iWell[72 - 1] = 0;  // Segment ID

            iWell[99 - 1] = compOrder(well);
        }
    } // IWell

    namespace SWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            // INTEHEAD(26) = NSWELZ
            return inteHead[26 - 1];
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

        template <class SWellArray>
        void staticContrib(SWellArray& sWell)
        {
            const auto dflt  = -1.0e+20f;
            const auto infty =  1.0e+20f;
            const auto zero  =  0.0f;
            const auto one   =  1.0f;
            const auto half  =  0.5f;

            // Initial data by Statoil ASA.
            const auto init = std::vector<float> { // 122 Items (0..121)
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

            const auto sz = static_cast<
                decltype(init.size())>(sWell.size());

            auto b = std::begin(init);
            auto e = b + std::min(init.size(), sz);

            std::copy(b, e, std::begin(sWell));
        }
    } // SWell

    namespace XWell {
        using SolnQuant = ::Opm::data::Rates::opt;

        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            // INTEHEAD(27) = NXWELZ
            return inteHead[27 - 1];
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

        bool activeGas(const ::Opm::Phases& phases)
        {
            return phases.active(::Opm::Phase::GAS);
        }

        bool activeOil(const ::Opm::Phases& phases)
        {
            return phases.active(::Opm::Phase::OIL);
        }

        bool activeWater(const ::Opm::Phases& phases)
        {
            return phases.active(::Opm::Phase::WATER);
        }

        template <class XWellArray>
        void assignProducer(const ::Opm::data::Well& xw,
                            const ::Opm::Phases&     phases,
                            XWellArray&              xWell)
        {
            using SolnQuant = ::Opm::data::Rates::opt;

            if (activeGas(phases)) {
                if (xw.rates.has(SolnQuant::gas)) {
                    xWell[2] = xw.rates.get(SolnQuant::gas);
                }
            }

            if (activeOil(phases)) {
                if (xw.rates.has(SolnQuant::oil)) {
                    xWell[0] = xw.rates.get(SolnQuant::oil);
                }
            }

            if (activeWater(phases)) {
                if (xw.rates.has(SolnQuant::wat)) {
                    xWell[1] = xw.rates.get(SolnQuant::wat);
                }
            }
        }

        template <class XWellArray>
        void dynamicContrib(const ::Opm::Well&      well,
                            const ::Opm::Phases&    phases,
                            const ::Opm::WellRates& wRates,
                            XWellArray&             xWell)
        {
            auto x = wRates.find(well.name());
            if (x == wRates.end()) { return; }

            const auto& xw = *x->second;
        }
    } // XWell

    namespace ZWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            // INTEHEAD(28) = NZWELZ
            return inteHead[28 - 1];
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
                        const std::size_t rptStep)
{
    const auto& wells = sched.getWells(rptStep);

    // Extract Static Contributions to IWell Array
    {
        const auto grpNames = groupNames(sched.getGroups());

        wellLoop(wells, [&grpNames, rptStep, this]
            (const Well& well, const std::size_t wellID) -> void
        {
            auto iw = this->iWell_[wellID];

            IWell::staticContrib(well, grpNames, this->nWGMax_, rptStep, iw);
        });
    }

    // Define Static Contributions to SWell Array.
    wellLoop(wells,
        [this](const Well& /* well */, const std::size_t wellID) -> void
    {
        auto sw = this->sWell_[wellID];

        SWell::staticContrib(sw);
    });

    // Define Static Contributions to ZWell Array.
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
captureDynamicWellData(const Phases&                 phases,
                       const Schedule&               sched,
                       const std::size_t             rptStep,
                       const ::Opm::data::WellRates& wRates)
{
    const auto& wells = sched.getWells(rptStep);

    wellLoop(wells, [this, &phases, &wRates]
        (const Well& well, const std::size_t wellID) -> void
    {
        auto xw = this->xWell_[wellID];

        XWell::dynamicContrib(well, phases, wRates, xw);
    });
}
