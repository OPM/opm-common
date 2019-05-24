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
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

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
            // All blanks.  Return empty.
            return "";
        }

        const auto e = s.find_last_not_of(" \t");
        assert ((e != std::string::npos) && "Logic Error");

        // Remove leading/trailing blanks.
        return s.substr(b, e - b + 1);
    }

    template <typename WellOp>
    void wellLoop(const std::vector<Opm::Well2>& wells,
                  WellOp&&                       wellOp)
    {
        for (auto nWell = wells.size(), wellID = 0*nWell;
             wellID < nWell; ++wellID)
        {
            const auto& well = wells[wellID];

            wellOp(well, wellID);
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

	std::map <const std::string, size_t>  currentGroupMapNameIndex(const Opm::Schedule& sched, const size_t simStep, const std::vector<int>& inteHead)
	{
	    const auto& groups = sched.getGroups(simStep);
	    // make group name to index map for the current time step
	    std::map <const std::string, size_t> groupIndexMap;
	    for (const auto* group : groups) {
		int ind = (group->name() == "FIELD")
			? inteHead[VI::intehead::NGMAXZ]-1 : group->seqIndex()-1;
		std::pair<const std::string, size_t> groupPair = std::make_pair(group->name(), ind);
		groupIndexMap.insert(groupPair);
	    }
	    return groupIndexMap;
        }

        int groupIndex(const std::string&              grpName,
                       const std::map <const std::string, size_t>&  currentGroupMapNameIndex)
        {
	    int ind = 0;
	    auto searchGTName = currentGroupMapNameIndex.find(grpName);
	    if (searchGTName != currentGroupMapNameIndex.end()) {
		ind = searchGTName->second + 1;
	    }
	    else {
		std::cout << "group Name: " << grpName << std::endl;
		throw std::invalid_argument( "Invalid group name" );
	    }
            return ind;
        }

        int wellType(const Opm::Well2& well)
        {
            using WTypeVal = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::WellType;
            Opm::SummaryState summaryState;

            if (well.isProducer()) {
                return WTypeVal::Producer;
            }

            using IType = ::Opm::WellInjector::TypeEnum;

            const auto itype = well.injectionControls(summaryState).injector_type;

            switch (itype) {
            case IType::OIL:   return WTypeVal::OilInj;
            case IType::WATER: return WTypeVal::WatInj;
            case IType::GAS:   return WTypeVal::GasInj;
            default:           return WTypeVal::WTUnk;
            }
        }

        int wellVFPTab(const Opm::Well2& well)
        {
            Opm::SummaryState summaryState;
            if (well.isInjector()) {
                return well.injectionControls(summaryState).vfp_table_number;
            }
            return well.productionControls(summaryState).vfp_table_number;
        }

        int ctrlMode(const Opm::Well2& well)
        {
            using WMCtrlVal = ::Opm::RestartIO::Helpers::VectorItems::IWell::Value::WellCtrlMode;
            Opm::SummaryState summaryState;

            if (well.isInjector()) {
                const auto& controls = well.injectionControls(summaryState);

                const auto wmctl = controls.cmode;
                const auto wtype = controls.injector_type;

                using CMode = ::Opm::WellInjector::ControlModeEnum;
                using WType = ::Opm::WellInjector::TypeEnum;

                switch (wmctl) {
                case CMode::RATE: {
                    switch (wtype) {
                    case WType::OIL:   return WMCtrlVal::OilRate;
                    case WType::WATER: return WMCtrlVal::WatRate;
                    case WType::GAS:   return WMCtrlVal::GasRate;
                    case WType::MULTI: return WMCtrlVal::WMCtlUnk;
                    }
                }
                    break;

                case CMode::RESV: return WMCtrlVal::ResVRate;
                case CMode::THP:  return WMCtrlVal::THP;
                case CMode::BHP:  return WMCtrlVal::BHP;
                case CMode::GRUP: return WMCtrlVal::Group;

                default:
                {
                    const auto stat = well.getStatus();

                    using WStat = ::Opm::WellCommon::StatusEnum;

                    if (stat == WStat::SHUT) {
                        return WMCtrlVal::Shut;
                    }
                }
                return WMCtrlVal::WMCtlUnk;
                }
            }
            else if (well.isProducer()) {
                const auto& controls = well.productionControls(summaryState);

                using CMode = ::Opm::WellProducer::ControlModeEnum;

                switch (controls.cmode) {
                case CMode::ORAT: return WMCtrlVal::OilRate;
                case CMode::WRAT: return WMCtrlVal::WatRate;
                case CMode::GRAT: return WMCtrlVal::GasRate;
                case CMode::LRAT: return WMCtrlVal::LiqRate;
                case CMode::RESV: return WMCtrlVal::ResVRate;
                case CMode::THP:  return WMCtrlVal::THP;
                case CMode::BHP:  return WMCtrlVal::BHP;
                case CMode::CRAT: return WMCtrlVal::CombRate;
                case CMode::GRUP: return WMCtrlVal::Group;

                default:
                {
                    const auto stat = well.getStatus();

                    using WStat = ::Opm::WellCommon::StatusEnum;

                    if (stat == WStat::SHUT) {
                        return WMCtrlVal::Shut;
                    }
                }
                return WMCtrlVal::WMCtlUnk;
                }
            }

            return WMCtrlVal::WMCtlUnk;
        }

        int compOrder(const Opm::Well2& well)
        {
            using WCO   = ::Opm::WellCompletion::CompletionOrderEnum;
            using COVal = ::Opm::RestartIO::Helpers::
                VectorItems::IWell::Value::CompOrder;

            switch (well.getWellConnectionOrdering()) {
            case WCO::TRACK: return COVal::Track;
            case WCO::DEPTH: return COVal::Depth;
            case WCO::INPUT: return COVal::Input;
            }

            return 0;
        }

        template <class IWellArray>
        void staticContrib(const Opm::Well2&               well,
                           const std::size_t               msWellID,
                           const std::map <const std::string, size_t>&  GroupMapNameInd,
                           IWellArray&                     iWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

            iWell[Ix::IHead] = well.getHeadI() + 1;
            iWell[Ix::JHead] = well.getHeadJ() + 1;

            // Connections
            {
                const auto& conn = well.getConnections();

                iWell[Ix::NConn]  = static_cast<int>(conn.size());

                if (well.isMultiSegment()) {
                    // Set top and bottom connections to zero for multi
                    // segment wells
                    iWell[Ix::FirstK] = 0;
                    iWell[Ix::LastK]  = 0;
                }
                else {
                    iWell[Ix::FirstK] = (iWell[Ix::NConn] == 0)
                        ? 0 : conn.get(0).getK() + 1;

                    iWell[Ix::LastK] = (iWell[Ix::NConn] == 0)
                        ? 0 : conn.get(conn.size() - 1).getK() + 1;
                }
            }

            iWell[Ix::Group] =
                groupIndex(trim(well.groupName()), GroupMapNameInd);

            iWell[Ix::WType]  = wellType  (well);
            iWell[Ix::VFPTab] = wellVFPTab(well);
            iWell[Ix::XFlow]  = well.getAllowCrossFlow() ? 1 : 0;

            // The following items aren't fully characterised yet, but
            // needed for restart of M2.  Will need further refinement.
            iWell[Ix::item18] = -100;
            iWell[Ix::item25] = -  1;
            iWell[Ix::item32] =    7;
            iWell[Ix::item48] = -  1;

            // Deliberate misrepresentation.  Function 'ctrlMode()' returns
            // the target control mode requested in the simulation deck.
            // This item is supposed to be the well's actual, active target
            // control mode in the simulator.
            iWell[Ix::ActWCtrl] = ctrlMode(well);

            if (well.predictionMode()) {
                // Well in prediction mode (WCONPROD, WCONINJE).  Assign
                // requested control mode for prediction.
                iWell[Ix::PredReqWCtrl] = iWell[Ix::ActWCtrl];
                iWell[Ix::HistReqWCtrl] = 0;
            }
            else {
                // Well controlled by observed rates/BHP (WCONHIST,
                // WCONINJH).  Assign requested control mode for history.
                iWell[Ix::PredReqWCtrl] = 0; // Possibly =1 instead.
                iWell[Ix::HistReqWCtrl] = iWell[Ix::ActWCtrl];
            }

            // Multi-segmented well information
            iWell[Ix::MsWID] = 0;  // MS Well ID (0 or 1..#MS wells)
            iWell[Ix::NWseg] = 0;  // Number of well segments
            if (well.isMultiSegment()) {
                iWell[Ix::MsWID] = static_cast<int>(msWellID);
                iWell[Ix::NWseg] =
                    well.getSegments().size();
            }

            iWell[Ix::CompOrd] = compOrder(well);
        }

                template <class IWellArray>
                void dynamicContribShut(IWellArray& iWell)
                {
                    using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

                    iWell[Ix::item9 ] = -1000;
                    iWell[Ix::item11] = -1000;
                }

            template <class IWellArray>
                void dynamicContribOpen(const Opm::data::Well& xw,
                                        IWellArray&            iWell)
            {
                using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

                const auto any_flowing_conn =
                    std::any_of(std::begin(xw.connections),
                                std::end  (xw.connections),
                                [](const Opm::data::Connection& c)
                                {
                                    return c.rates.any();
                                });

                iWell[Ix::item9] = any_flowing_conn
                    ? iWell[Ix::ActWCtrl] : -1;

                iWell[Ix::item11] = 1;
            }
        } // IWell

    namespace SWell {
        std::size_t entriesPerWell(const std::vector<int>& inteHead)
        {
            assert ((inteHead[VI::intehead::NSWELZ] > 121) &&
                    "SWEL must allocate at least 122 elements per well");

            return inteHead[VI::intehead::NSWELZ];
        }

        float datumDepth(const Opm::Well2&  well)
        {
            if (well.isMultiSegment()) {
                // Datum depth for multi-segment wells is
                // depth of top-most segment.
                return well.getSegments().depthTopSegment();
            }

            // Not a multi-segment well--i.e., this is a regular
            // well.  Use well's reference depth.
            return well.getRefDepth();
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
                zero , one  , zero , zero,  zero , zero ,    //  30.. 35  ( 5)
                zero , zero , zero , zero , zero , zero ,    //  36.. 41  ( 6)
                zero , zero , zero , zero , zero , zero ,    //  42.. 47  ( 7)
                zero , zero , zero , zero , zero , zero ,    //  48.. 53  ( 8)
                zero,  zero , zero , zero , zero , zero ,    //  54.. 59  ( 9)
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
        void staticContrib(const Opm::Well2&      well,
                           const Opm::UnitSystem& units,
                           const ::Opm::SummaryState& smry,
                           SWellArray&            sWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;
            using M = ::Opm::UnitSystem::measure;

            auto swprop = [&units](const M u, const double x) -> float
            {
                return static_cast<float>(units.from_si(u, x));
            };

            assignDefaultSWell(sWell);

            if (well.isProducer()) {
                const auto& pc = well.productionControls(smry);
                const auto& predMode = well.predictionMode();

                if ((pc.oil_rate != 0.0) || (!predMode)) {
                    sWell[Ix::OilRateTarget] =
                        swprop(M::liquid_surface_rate, pc.oil_rate);
                }

                if ((pc.water_rate != 0.0) || (!predMode)) {
                    sWell[Ix::WatRateTarget] =
                        swprop(M::liquid_surface_rate, pc.water_rate);
                }

                if ((pc.gas_rate != 0.0) || (!predMode)) {
                    sWell[Ix::GasRateTarget] =
                        swprop(M::gas_surface_rate, pc.gas_rate);
                    sWell[Ix::HistGasRateTarget] = sWell[Ix::GasRateTarget];
                }

                if (pc.liquid_rate != 0.0 || (!predMode)) {
                    sWell[Ix::LiqRateTarget] =
                        swprop(M::liquid_surface_rate, pc.liquid_rate);
                    sWell[Ix::HistLiqRateTarget] = sWell[Ix::LiqRateTarget];
                }
                else  {
                    sWell[Ix::LiqRateTarget] =
                        swprop(M::liquid_surface_rate, pc.oil_rate + pc.water_rate);
                }

                if (pc.resv_rate != 0.0)  {
                    sWell[Ix::ResVRateTarget] =
                        swprop(M::rate, pc.resv_rate);
                }
                else if ((smry.has("WVPR:" + well.name())) && (!predMode)) {
                    // Write out summary voidage production rate if
                    // target/limit is not set
                    auto vr = static_cast<float>(smry.get("WVPR:" + well.name()));
                    if (vr != 0.0) {
                        sWell[Ix::ResVRateTarget] = vr;
                    }
                }

                sWell[Ix::THPTarget] = pc.thp_limit != 0.0
                    ? swprop(M::pressure, pc.thp_limit)
                    : 0.0;

                sWell[Ix::BHPTarget] = pc.bhp_limit != 0.0
                    ? swprop(M::pressure, pc.bhp_limit)
                    : swprop(M::pressure, 1.0*::Opm::unit::atm);
                sWell[Ix::HistBHPTarget] = sWell[Ix::BHPTarget];
            }
            else if (well.isInjector()) {
                const auto& ic = well.injectionControls(smry);

                using IP = ::Opm::WellInjector::ControlModeEnum;
                using IT = ::Opm::WellInjector::TypeEnum;

                if (ic.hasControl(IP::RATE)) {
                    if (ic.injector_type == IT::OIL) {
                        sWell[Ix::OilRateTarget] =
                            swprop(M::liquid_surface_rate, ic.surface_rate);
                    }
                    if (ic.injector_type == IT::WATER) {
                        sWell[Ix::WatRateTarget] =
                            swprop(M::liquid_surface_rate, ic.surface_rate);
                        sWell[Ix::HistLiqRateTarget] = sWell[Ix::WatRateTarget];
                    }
                    if (ic.injector_type == IT::GAS) {
                        sWell[Ix::GasRateTarget] =
                            swprop(M::gas_surface_rate, ic.surface_rate);
                        sWell[Ix::HistGasRateTarget] = sWell[Ix::GasRateTarget];
                    }
                }

                if (ic.hasControl(IP::RESV)) {
                    sWell[Ix::ResVRateTarget] =
                        swprop(M::rate, ic.reservoir_rate);
                }

                if (ic.hasControl(IP::THP)) {
                    sWell[Ix::THPTarget] = swprop(M::pressure, ic.thp_limit);
                }

                sWell[Ix::BHPTarget] = ic.hasControl(IP::BHP)
                    ? swprop(M::pressure, ic.bhp_limit)
                    : swprop(M::pressure, 1.0E05*::Opm::unit::psia);
                sWell[Ix::HistBHPTarget] = sWell[Ix::BHPTarget];
            }

            sWell[Ix::DatumDepth] = swprop(M::length, datumDepth(well));
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
        void staticContrib(const ::Opm::Well2&    well,
                           const Opm::UnitSystem& units,
                           XWellArray&            xWell)
        {
            using M  = ::Opm::UnitSystem::measure;
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;
            Opm::SummaryState summaryState;

            const auto bhpTarget = well.isInjector()
                ? well.injectionControls(summaryState).bhp_limit
                : well.productionControls(summaryState).bhp_limit;

            xWell[Ix::BHPTarget] = units.from_si(M::pressure, bhpTarget);
        }

        template <class XWellArray>
        void assignProducer(const std::string&         well,
                            const ::Opm::SummaryState& smry,
                            XWellArray&                xWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

            auto get = [&smry, &well](const std::string& vector)
            {
                const auto key = vector + ':' + well;

                return smry.has(key) ? smry.get(key) : 0.0;
            };

            xWell[Ix::OilPrRate] = get("WOPR");
            xWell[Ix::WatPrRate] = get("WWPR");
            xWell[Ix::GasPrRate] = get("WGPR");

            xWell[Ix::LiqPrRate] = xWell[Ix::OilPrRate]
                                 + xWell[Ix::WatPrRate];

            xWell[Ix::VoidPrRate] = get("WVPR");

            xWell[Ix::FlowBHP] = get("WBHP");
            xWell[Ix::WatCut]  = get("WWCT");
            xWell[Ix::GORatio] = get("WGOR");

            xWell[Ix::OilPrTotal]  = get("WOPT");
            xWell[Ix::WatPrTotal]  = get("WWPT");
            xWell[Ix::GasPrTotal]  = get("WGPT");
            xWell[Ix::VoidPrTotal] = get("WVPT");

            // Not fully characterised.
            xWell[Ix::item37] = xWell[Ix::WatPrRate];
            xWell[Ix::item38] = xWell[Ix::GasPrRate];

            xWell[Ix::HistOilPrTotal] = get("WOPTH");
            xWell[Ix::HistWatPrTotal] = get("WWPTH");
            xWell[Ix::HistGasPrTotal] = get("WGPTH");
        }

        template <class GetSummaryVector, class XWellArray>
        void assignCommonInjector(GetSummaryVector& get,
                                  XWellArray&       xWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

            xWell[Ix::FlowBHP] = get("WBHP");

            // Note: Assign both water and gas cumulatives to support
            // case of well alternating between injecting water and gas.
            xWell[Ix::WatInjTotal]     = get("WWIT");
            xWell[Ix::GasInjTotal]     = get("WGIT");
            xWell[Ix::HistWatInjTotal] = get("WWITH");
            xWell[Ix::HistGasInjTotal] = get("WGITH");
        }

        template <class XWellArray>
        void assignWaterInjector(const std::string&         well,
                                 const ::Opm::SummaryState& smry,
                                 XWellArray&                xWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

            auto get = [&smry, &well](const std::string& vector)
            {
                const auto key = vector + ':' + well;

                return smry.has(key) ? smry.get(key) : 0.0;
            };

            assignCommonInjector(get, xWell);

            // Injection rates reported as negative.
            xWell[Ix::WatPrRate] = -get("WWIR");
            xWell[Ix::LiqPrRate] = xWell[Ix::WatPrRate];

            // Not fully characterised.
            xWell[Ix::item37] = xWell[Ix::WatPrRate];

            xWell[Ix::WatVoidPrRate] = -get("WWVIR");
        }

        template <class XWellArray>
        void assignGasInjector(const std::string&         well,
                               const ::Opm::SummaryState& smry,
                               XWellArray&                xWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

            auto get = [&smry, &well](const std::string& vector)
            {
                const auto key = vector + ':' + well;

                return smry.has(key) ? smry.get(key) : 0.0;
            };

            assignCommonInjector(get, xWell);

            // Injection rates reported as negative production rates.
            xWell[Ix::GasPrRate]  = -get("WGIR");
            xWell[Ix::VoidPrRate] = -get("WGVIR");

            xWell[Ix::GasFVF] = (std::abs(xWell[Ix::GasPrRate]) > 0.0)
                ? xWell[Ix::VoidPrRate] / xWell[Ix::GasPrRate]
                : 0.0;

            if (std::isnan(xWell[Ix::GasFVF])) {
                xWell[Ix::GasFVF] = 0.0;
            }

            // Not fully characterised.
            xWell[Ix::item38] = xWell[Ix::GasPrRate];

            xWell[Ix::GasVoidPrRate] = xWell[Ix::VoidPrRate];
        }

        template <class XWellArray>
        void dynamicContrib(const ::Opm::Well2&        well,
                            const ::Opm::SummaryState& smry,
                            XWellArray&                xWell)
        {
            if (well.isProducer()) {
                assignProducer(well.name(), smry, xWell);
            }
            else if (well.isInjector()) {
                using IType = ::Opm::WellInjector::TypeEnum;
                const auto itype = well.injectionControls(smry).injector_type;

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
        void staticContrib(const Opm::Well2& well, ZWellArray& zWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::ZWell::index;

            zWell[Ix::WellName] = well.name();
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
                        const std::size_t sim_step,
                        const ::Opm::SummaryState&  smry,
                        const std::vector<int>& inteHead)
{
    const auto& wells = sched.getWells2(sim_step);

    // Static contributions to IWEL array.
    {
        //const auto grpNames = groupNames(sched.getGroups());
        const auto groupMapNameIndex = IWell::currentGroupMapNameIndex(sched, sim_step, inteHead);
        auto msWellID       = std::size_t{0};

        wellLoop(wells, [&groupMapNameIndex, &msWellID, this]
                 (const Well2& well, const std::size_t wellID) -> void
                 {
                     msWellID += well.isMultiSegment();  // 1-based index.
                     auto iw   = this->iWell_[wellID];

                     IWell::staticContrib(well, msWellID, groupMapNameIndex, iw);
                 });
    }

    // Static contributions to SWEL array.
    wellLoop(wells, [&units, &smry, this]
             (const Well2& well, const std::size_t wellID) -> void
             {
                 auto sw = this->sWell_[wellID];

                 SWell::staticContrib(well, units, smry, sw);
             });

    // Static contributions to XWEL array.
    wellLoop(wells, [&units, this]
             (const Well2& well, const std::size_t wellID) -> void
             {
                 auto xw = this->xWell_[wellID];

                 XWell::staticContrib(well, units, xw);
             });

    // Static contributions to ZWEL array.
    wellLoop(wells,
             [this](const Well2& well, const std::size_t wellID) -> void
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
    const auto& wells = sched.getWells2(sim_step);

    // Dynamic contributions to IWEL array.
    wellLoop(wells, [this, &xw]
             (const Well2& well, const std::size_t wellID) -> void
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
    wellLoop(wells, [this, &smry]
        (const Well2& well, const std::size_t wellID) -> void
    {
        auto xw = this->xWell_[wellID];

        XWell::dynamicContrib(well, smry, xw);
    });
}
