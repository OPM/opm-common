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

#include <opm/output/eclipse/SummaryState.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
//#include <opm/output/data/Wells.hpp>

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
	    if (searchGTName != currentGroupMapNameIndex.end()) 
	    {
		ind = searchGTName->second + 1;	
	    }
	    else 
	    {
		std::cout << "group Name: " << grpName << std::endl;
		throw std::invalid_argument( "Invalid group name" );
	    }
	return ind;
      }
        /*int groupIndex(const std::string&              grpName,
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
        }  */

        int wellType(const Opm::Well&  well,
                     const std::size_t sim_step)
        {
            using WTypeVal = ::Opm::RestartIO::Helpers::
                VectorItems::IWell::Value::WellType;

            if (well.isProducer(sim_step)) {
                return WTypeVal::Producer;
            }

            using IType = ::Opm::WellInjector::TypeEnum;

            const auto itype = well
                .getInjectionProperties(sim_step).injectorType;

            switch (itype) {
            case IType::OIL:   return WTypeVal::OilInj;
            case IType::WATER: return WTypeVal::WatInj;
            case IType::GAS:   return WTypeVal::GasInj;
            default:           return WTypeVal::WTUnk;
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
            using WMCtrlVal = ::Opm::RestartIO::Helpers::
                VectorItems::IWell::Value::WellCtrlMode;

            {
                const auto stat = well.getStatus(sim_step);

                using WStat = ::Opm::WellCommon::StatusEnum;

                if ((stat == WStat::SHUT) || (stat == WStat::STOP)) {
                    return WMCtrlVal::Shut;
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
                    return WMCtrlVal::WMCtlUnk;
                }
            }
            else if (well.isProducer(sim_step)) {
                const auto& prop = well
                    .getProductionProperties(sim_step);

                using CMode = ::Opm::WellProducer::ControlModeEnum;

                switch (prop.controlMode) {
                case CMode::ORAT: return WMCtrlVal::OilRate;
                case CMode::WRAT: return WMCtrlVal::WatRate;
                case CMode::GRAT: return WMCtrlVal::GasRate;
                case CMode::LRAT: return WMCtrlVal::LiqRate;
                case CMode::RESV: return WMCtrlVal::ResVRate;
                case CMode::THP:  return WMCtrlVal::THP;
                case CMode::BHP:  return WMCtrlVal::BHP;
                case CMode::CRAT: return WMCtrlVal::CombRate;
                case CMode::GRUP: return WMCtrlVal::Group;

                default: return WMCtrlVal::WMCtlUnk;
                }
            }

            return WMCtrlVal::WMCtlUnk;
        }

        int compOrder(const Opm::Well& well)
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
        void staticContrib(const Opm::Well&                well,
                           const std::size_t               msWellID,
                           const std::map <const std::string, size_t>&  GroupMapNameInd,
			   /*const std::vector<std::string>& groupNames,*/
                           const int                       maxGroups,
                           const std::size_t               sim_step,
                           IWellArray&                     iWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::IWell::index;

            iWell[Ix::IHead] = well.getHeadI(sim_step) + 1;
            iWell[Ix::JHead] = well.getHeadJ(sim_step) + 1;

            // Connections
            {
                const auto& conn = well.getConnections(sim_step);

                iWell[Ix::NConn]  = static_cast<int>(conn.size());
                // IWEL(5) = Number of active cells connected to well.
                iWell[5 - 1] = static_cast<int>(conn.size());
		
		if (well.isMultiSegment(sim_step))  
		{
		    // Set top and bottom connections to zero for multi segment wells
		  iWell[Ix::FirstK] = 0;
		  iWell[Ix::LastK] = 0;
		}
		else
		{
		  iWell[Ix::FirstK] = (iWell[Ix::NConn] == 0)
                    ? 0 : conn.get(0).getK() + 1;

		  iWell[Ix::LastK] = (iWell[Ix::NConn] == 0)
		}
            }

            iWell[Ix::Group] =
	     groupIndex(trim(well.getGroupName(sim_step)),
                GroupMapNameInd);

            iWell[Ix::WType]  = wellType  (well, sim_step);
            iWell[Ix::WCtrl]  = ctrlMode  (well, sim_step);
            iWell[Ix::VFPTab] = wellVFPTab(well, sim_step);

            // The following items aren't fully characterised yet, but
            // needed for restart of M2.  Will need further refinement.
            iWell[Ix::item18] = -100;
            iWell[Ix::item25] = -  1;
            iWell[Ix::item32] =    7;
            iWell[Ix::item48] = -  1;

            iWell[Ix::item50] = iWell[Ix::WCtrl];

            // Multi-segmented well information
            iWell[Ix::MsWID] = 0;  // MS Well ID (0 or 1..#MS wells)
            iWell[Ix::NWseg] = 0;  // Number of well segments
            if (well.isMultiSegment(sim_step)) {
                iWell[Ix::MsWID] = static_cast<int>(msWellID);
                iWell[Ix::NWseg] =
                    well.getWellSegments(sim_step).size();
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
                ? iWell[Ix::WCtrl] : -1;

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
                zero , zero , zero , zero ,infty , zero,    //   0..  5  ( 0)
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
			   const ::Opm::SummaryState& smry,
                           SWellArray&            sWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::SWell::index;
            using M = ::Opm::UnitSystem::measure;

            auto swprop = [&units](const M u, const double x) -> float
            {
                return static_cast<float>(units.from_si(u, x));
            };

	    auto get = [&smry, &well](const std::string& vector)
            {
                return smry.get(vector + ':' + well.name());
            };
	    
            assignDefaultSWell(sWell);

            if (well.isProducer(sim_step)) {
                const auto& pp = well.getProductionProperties(sim_step);

                using PP = ::Opm::WellProducer::ControlModeEnum;

                if (pp.OilRate != 0.0) {
                    sWell[Ix::OilRateTarget] =
                        swprop(M::liquid_surface_rate, pp.OilRate);
		}

                if (pp.WaterRate != 0.0) {
                    sWell[Ix::WatRateTarget] =
                        swprop(M::liquid_surface_rate, pp.WaterRate);
                }

                if (pp.GasRate != 0.0) {
                    sWell[Ix::GasRateTarget] =
                        swprop(M::gas_surface_rate, pp.GasRate);
                }

                if (pp.LiquidRate != 0.0) {
                    sWell[Ix::LiqRateTarget] =
                        swprop(M::liquid_surface_rate, pp.LiquidRate);
                }
                else  {
                    sWell[Ix::LiqRateTarget] =
                        swprop(M::liquid_surface_rate, pp.OilRate + pp.WaterRate);
                }

                if (pp.ResVRate != 0.0) {
                    sWell[Ix::ResVRateTarget] =
                        swprop(M::rate, pp.ResVRate);
                }
                //write out summary voidage production rate if target/limit is not set
		else {
                    sWell[Ix::ResVRateTarget] = get("WVPR");
                }

                if (pp.THPLimit != 0.0) {
                    sWell[Ix::THPTarget] =
                        swprop(M::pressure, pp.THPLimit);
                }
           
		sWell[Ix::BHPTarget] = pp.BHPLimit != 0.0
                    ? swprop(M::pressure, pp.BHPLimit)
                    : swprop(M::pressure, 100.0e3*::Opm::unit::psia);
            }
            else if (well.isInjector(sim_step)) {
                const auto& ip = well.getInjectionProperties(sim_step);

                using IP = ::Opm::WellInjector::ControlModeEnum;
		using IT = ::Opm::WellInjector::TypeEnum;

		if (ip.hasInjectionControl(IP::RATE)) {
		    if (ip.injectorType == IT::OIL) {
			sWell[Ix::OilRateTarget] = swprop(M::gas_surface_rate, ip.surfaceInjectionRate);
		    }
		    if (ip.injectorType == IT::WATER) {
			sWell[Ix::WatRateTarget] = swprop(M::gas_surface_rate, ip.surfaceInjectionRate);
		    }
		    if (ip.injectorType == IT::GAS) {
			sWell[Ix::GasRateTarget] = swprop(M::gas_surface_rate, ip.surfaceInjectionRate);
		    }
                }

		if (ip.hasInjectionControl(IP::RESV)) {
		    sWell[x::ResVRateTarget] = swprop(M::gas_surface_rate, ip.reservoirInjectionRate);
                }		

                if (ip.hasInjectionControl(IP::THP)) {
                    sWell[Ix::THPTarget] = swprop(M::pressure, ip.THPLimit);
                }

                sWell[Ix::BHPTarget] = ip.hasInjectionControl(IP::BHP)
                    ? swprop(M::pressure, ip.BHPLimit)
                    : swprop(M::pressure, 1.0*::Opm::unit::atm);
            }

            sWell[Ix::DatumDepth] =
                swprop(M::length, datumDepth(well, sim_step));
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
            using M  = ::Opm::UnitSystem::measure;
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

            const auto bhpTarget = well.isInjector(sim_step)
                ? well.getInjectionProperties (sim_step).BHPLimit
                : well.getProductionProperties(sim_step).BHPLimit;

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
                return smry.get(vector + ':' + well);
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
        }

        template <class XWellArray>
        void assignWaterInjector(const std::string&         well,
                                 const ::Opm::SummaryState& smry,
                                 XWellArray&                xWell)
        {
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XWell::index;

            auto get = [&smry, &well](const std::string& vector)
            {
                return smry.get(vector + ':' + well);
            };

            // Injection rates reported as negative, cumulative
            // totals as positive.
            xWell[Ix::WatPrRate] = -get("WWIR");
            xWell[Ix::LiqPrRate] = xWell[Ix::WatPrRate];

            xWell[Ix::FlowBHP] = get("WBHP");

            xWell[Ix::WatInjTotal] = get("WWIT");

            xWell[Ix::item37] = xWell[Ix::WatPrRate];
            xWell[Ix::item82] = xWell[Ix::WatInjTotal];

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
                return smry.get(vector + ':' + well);
            };

            // Injection rates reported as negative production rates,
            // cumulative injection totals as positive.
            xWell[Ix::GasPrRate]  = -get("WGIR");
            xWell[Ix::VoidPrRate] = -get("WGVIR");

            xWell[Ix::FlowBHP] = get("WBHP");

            xWell[Ix::GasInjTotal] = get("WGIT");

            xWell[Ix::GasFVF] = xWell[Ix::VoidPrRate]
                              / xWell[Ix::GasPrRate];

            // Not fully characterised.
            xWell[Ix::item38] = xWell[Ix::GasPrRate];
            xWell[Ix::item83] = xWell[Ix::GasInjTotal];

            xWell[Ix::GasVoidPrRate] = xWell[Ix::VoidPrRate];
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
    const auto& wells = sched.getWells(sim_step);

    // Static contributions to IWEL array.
    {
        //const auto grpNames = groupNames(sched.getGroups());
	const auto groupMapNameIndex = IWell::currentGroupMapNameIndex(sched, sim_step, inteHead);
        auto msWellID       = std::size_t{0};

        wellLoop(wells, [&groupMapNameIndex, &msWellID, sim_step, this]
            (const Well& well, const std::size_t wellID) -> void
        {
            msWellID += well.isMultiSegment(sim_step);  // 1-based index.
            auto iw   = this->iWell_[wellID];

            IWell::staticContrib(well, msWellID, groupMapNameIndex,
                                 this->nWGMax_, sim_step, iw);
        });
    }

    // Static contributions to SWEL array.
    wellLoop(wells, [&units, sim_step, &smry, this]
        (const Well& well, const std::size_t wellID) -> void
    {
        auto sw = this->sWell_[wellID];

        SWell::staticContrib(well, units, sim_step, smry, sw);
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
