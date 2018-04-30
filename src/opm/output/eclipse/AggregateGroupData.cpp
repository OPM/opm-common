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

#include <opm/output/eclipse/AggregateGroupData.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateGroupData
// ---------------------------------------------------------------------
//namespace Opm {
//    class Phases;
//    class Schedule;
//    class Group;
//} // Opm

namespace {
  
  int nigrpz(const std::vector<int>& inteHead)
    {
        // INTEHEAD(36) = NIGRPZ
        return inteHead[36];
    }

    int nsgrpz(const std::vector<int>& inteHead)
    {
        return inteHead[37];
    }
    
    int nxgrpz(const std::vector<int>& inteHead)
    {
        return inteHead[38];
    }
    
    int nzgrpz(const std::vector<int>& inteHead)
    {
        return inteHead[39];
    }
    // maximum number of groups
    int ngmaxz(const std::vector<int>& inteHead)
    {
        return inteHead[20];
    }
    // maximum number of wells in any group
    int nwgmax(const std::vector<int>& inteHead)
    {
        return inteHead[19];
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

    const int groupType(const Opm::Schedule& sched,
		      const Opm::Group& group,
		      const std::size_t rptStep)
    {
	const std::string& groupName = group.name();
	if (!sched.hasGroup(groupName))
            throw std::invalid_argument("No such group: " + groupName);
        {
            if (group.hasBeenDefined( rptStep )) {
                const auto& groupTree = sched.getGroupTree( rptStep );
                const auto& childGroups = groupTree.children( groupName );

                if (childGroups.size()) {
		    return 1;
                } 
                else {
		    return 0;
		}
            }
            return 0;
        }
    }
    
    template <typename GroupOp>
    void groupLoop(const std::vector<const Opm::Group*>& groups,
                  GroupOp&&                             groupOp)
    {
        auto groupID = std::size_t{0};
        for (const auto* group : groups) {
            groupID += 1;

            if (group == nullptr) { continue; }

            groupOp(*group, groupID - 1);
        }
    }

    std::map <size_t, std::string>  currentGroupMapIndexName(const Opm::Schedule& sched, const size_t rptStep)
    {
	//const std::vector< const Opm::Group* > groups = sched.getGroups(rptStep);
	const auto& groups = sched.getGroups(rptStep);
	// make group index for current report step
	std::map <size_t, std::string> groupIndexMap;
	for (const auto* group : groups) {
	    const std::pair<size_t, std::string> groupPair = std::make_pair(group->seqIndex(), group->name()); 
	    groupIndexMap.insert(groupPair);
	}
	return groupIndexMap;
    }
    
    std::map <std::string, size_t>  currentGroupMapNameIndex(const Opm::Schedule& sched, const size_t rptStep)
    {
	//const std::vector< const Opm::Group* > groups = sched.getGroups(rptStep);
	const auto& groups = sched.getGroups(rptStep);
	// make group index for current report step
	std::map <std::string, size_t> groupIndexMap;
	for (const auto* group : groups) {
	    std::pair<std::string, size_t> groupPair = std::make_pair(group->name(), group->seqIndex()); 
	    groupIndexMap.insert(groupPair);
	}
	return groupIndexMap;
    }
  
    const std::map <std::string, size_t>  currentWellIndex(const Opm::Schedule& sched, const size_t rptStep)
    {
	const auto& wells = sched.getWells(rptStep);
	//const std::vector< const Opm::Well* > wells = sched.getWells(rptStep);
	// make group index for current report step
	std::map <std::string, size_t> wellIndexMap;
	for (const auto* well : wells) {
	    std::pair<std::string, size_t> wellPair = std::make_pair(well->name(), well->seqIndex()); 
	    wellIndexMap.insert(wellPair);
	}
	return wellIndexMap;
    }

#if 0  
    std::vector< const Opm::Group* > currentGroups(Opm::Schedule& sched, size_t rptStep)
    {
	const std::vector< const Opm::Group* >  groups = sched.getGroups(rptStep);
	return groups;
    }
#endif     
    const int currentGroupLevel(const Opm::Schedule& sched, const Opm::Group& group, const size_t rptStep)
    {
	int level = 0;
	const std::vector< const Opm::Group* >  groups = sched.getGroups(rptStep);
      	const std::string& groupName = group.name();
	if (!sched.hasGroup(groupName))
            throw std::invalid_argument("No such group: " + groupName);
        {
            if (group.hasBeenDefined( rptStep )) {
                const auto& groupTree = sched.getGroupTree( rptStep );
		//find group level - field level is 0
		std::string tstGrpName = groupName;
		while (((tstGrpName.size())>0) && (!(tstGrpName=="FIELD"))) {
		    std::string curParent = groupTree.parent(tstGrpName);
		    level+=1;
		    tstGrpName = curParent;
		}
		return level;
	    }
	    else {
		throw std::invalid_argument("actual group has not been defined at report time: " + rptStep );	      	      
	    }
	}
	return level;
    }
    
    namespace IGrp {
	std::size_t entriesPerGroup(const std::vector<int>& inteHead)
        {
            // INTEHEAD[36] = NIGRPZ
            return inteHead[36];
        }

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

            return WV {
                WV::NumWindows{ ngmaxz(inteHead) },
                WV::WindowSize{ entriesPerGroup(inteHead) }
            };
        }



        template <class IGrpArray>
        void staticContrib( const Opm::Schedule& 	sched,
			    const Opm::Group&         	group,
			    const int                  	nwgmax,
			    const std::size_t         	rptStep,
			    IGrpArray&             	igrp)
        {
	  // find the number of wells or child groups belonging to a group and store in 
	  // location nwgmax +1 in the igrp array
	  
	    const auto& childGroups = sched.getChildGroups(group.name(), rptStep);
	    const auto& childWells  = sched.getChildWells(group.name(), rptStep);
	    const auto& groupMapIndexName = currentGroupMapIndexName(sched, rptStep);
	    const auto& groupMapNameIndex = currentGroupMapNameIndex(sched, rptStep);
	    if ((childGroups.size() == 0) && (childWells.size()==0)) 
		throw std::invalid_argument("group has neither wells nor child groups" + group.name());
            int igrpCount = 0;
	    if (childGroups.size() == 0) {
		//group has child wells
		for (const auto* well : childWells ) {
		    igrp[igrpCount] = well->seqIndex()+1;
		    igrpCount+=1;
		}
	    }
	    else {
		//group has child groups
		for (const auto& group : childGroups ) {
		    igrp[igrpCount] = group->seqIndex() + 1;
		    igrpCount+=1;
		}
	    }
	    //assign the number of child wells or child groups to 
	    // location nwgmax
	    igrp[nwgmax] =  (childGroups.size() == 0)
                    ? childWells.size() : childGroups.size();
	    
	    // find the group type (well group (type 0) or node group (type 1) and store the type in  
	    // location nwgmax + 26
	    const auto grpType = groupType(sched, group, rptStep);
	    igrp[nwgmax+26] = grpType;
	    
	    //find group level ("FIELD" is level 0) and store the level in 
	    //location nwgmax + 27
	    const auto grpLevel = currentGroupLevel(sched, group, rptStep);
	    igrp[nwgmax+27] = grpLevel;
	    
	    //find parent group and store index of parent group in
	    //location nwgmax + 28
	    const auto& groupTree = sched.getGroupTree( rptStep );
	    const std::string& parent = groupTree.parent(group.name());
	    if (parent.size() == 0) 
		throw std::invalid_argument("parent group does not exist" + group.name());
	    const auto itr = groupMapNameIndex.find(parent);
	    igrp[nwgmax+28] = itr->second;
	    
        }
    } // Igrp

    namespace SGrp {
        std::size_t entriesPerGroup(const std::vector<int>& inteHead)
        {
            // INTEHEAD[37] = NSGRPZ
            return inteHead[37];
        }

        Opm::RestartIO::Helpers::WindowedArray<float>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<float>;

            return WV {
                WV::NumWindows{ ngmaxz(inteHead) },
                WV::WindowSize{ entriesPerGroup(inteHead) }
            };
        }
    }
#if 0
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
#endif
    namespace XGrp {
        using SolnQuant = ::Opm::data::Rates::opt;

        std::size_t entriesPerGroup(const std::vector<int>& inteHead)
        {
            // INTEHEAD[38] = NXGRPZ
            return inteHead[38];
        }

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<double>;

            return WV {
                WV::NumWindows{ ngmaxz(inteHead) },
                WV::WindowSize{ entriesPerGroup(inteHead) }
            };
        }
    }
#if 0
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
#endif

    namespace ZGrp {
        std::size_t entriesPerGroup(const std::vector<int>& inteHead)
        {
            // INTEHEAD[39] = NZGRPZ
            return inteHead[39];
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
                WV::NumWindows{ ngmaxz(inteHead) },
                WV::WindowSize{ entriesPerGroup(inteHead) }
            };
        }
    }

#if 0

        template <class ZWellArray>
        void staticContrib(const Opm::Well& well, ZWellArray& zWell)
        {
            zWell[1 - 1] = well.name();
        }
    } // ZWell
#endif
} // Anonymous




// =====================================================================

Opm::RestartIO::Helpers::AggregateGroupData::
AggregateGroupData(const std::vector<int>& inteHead)
    : iGroup_ (IGrp::allocate(inteHead))
    , sGroup_ (SGrp::allocate(inteHead))
    , xGroup_ (XGrp::allocate(inteHead))
    , zGroup_ (ZGrp::allocate(inteHead))
    , nWGMax_(nwgmax(inteHead))
    , nGMaxz_(ngmaxz(inteHead))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateGroupData::
captureDeclaredGroupData(const Schedule&   sched,
                        const std::size_t rptStep)
{
    const auto& groups = sched.getGroups(rptStep);

    // 
    {
      //const auto& groupIndMap = currentGroupMapIndexName( sched, rptStep);
      //const auto& grpNames = groupNames(groups);

        groupLoop(groups, [sched, rptStep, this]
            (const Group& group, const std::size_t groupID) -> void
        {
            auto ig = this->iGroup_[groupID];

            IGrp::staticContrib(sched, group, this->nGMaxz_, rptStep, ig);
        });
    }
    
#if 0
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
#endif
}

// ---------------------------------------------------------------------
#if 0
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
#endif 