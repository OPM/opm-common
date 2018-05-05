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
#include <opm/output/eclipse/WriteRestartHelpers.hpp>
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
#include <iostream>

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateGroupData
// ---------------------------------------------------------------------


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
		      const std::size_t simStep)
    {
	const std::string& groupName = group.name();
	if (!sched.hasGroup(groupName))
            throw std::invalid_argument("No such group: " + groupName);
        {
            if (group.hasBeenDefined( simStep )) {
                const auto& groupTree = sched.getGroupTree( simStep );
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

    std::map <size_t, const Opm::Group*>  currentGroupMapIndexGroup(const Opm::Schedule& sched, const size_t simStep, const std::vector<int>& inteHead)
    {
	//const std::vector< const Opm::Group* > groups = sched.getGroups(simStep);
	const auto& groups = sched.getGroups(simStep);
	// make group index for current report step
	std::map <size_t, const Opm::Group*> groupIndexMap;
	for (const auto* group : groups) {
	    int ind = (group->name() == "FIELD")
	    ? ngmaxz(inteHead)-1 : group->seqIndex()-1;
	    const std::pair<size_t, const Opm::Group*> groupPair = std::make_pair(static_cast<size_t>(ind), group); 
	    groupIndexMap.insert(groupPair);
	}
	return groupIndexMap;
    }

    std::map <std::string, size_t>  currentGroupMapNameIndex(const Opm::Schedule& sched, const size_t simStep, const std::vector<int>& inteHead)
    {
	//const std::vector< const Opm::Group* > groups = sched.getGroups(simStep);
	const auto& groups = sched.getGroups(simStep);
	// make group index for current report step
	std::map <std::string, size_t> groupIndexMap;
	for (const auto* group : groups) {
	    int ind = (group->name() == "FIELD")
                    ? ngmaxz(inteHead)-1 : group->seqIndex()-1;
	    std::pair<std::string, size_t> groupPair = std::make_pair(group->name(), ind); 
	    groupIndexMap.insert(groupPair);
	}
	return groupIndexMap;
    }

    const std::map <std::string, size_t>  currentWellIndex(const Opm::Schedule& sched, const size_t simStep)
    {
	const auto& wells = sched.getWells(simStep);
	//const std::vector< const Opm::Well* > wells = sched.getWells(simStep);
	// make group index for current report step
	std::map <std::string, size_t> wellIndexMap;
	for (const auto* well : wells) {
	    std::pair<std::string, size_t> wellPair = std::make_pair(well->name(), well->seqIndex()); 
	    wellIndexMap.insert(wellPair);
	}
	return wellIndexMap;
    }

#if 0
    std::vector< const Opm::Group* > currentGroups(Opm::Schedule& sched, size_t simStep)
    {
	const std::vector< const Opm::Group* >  groups = sched.getGroups(simStep);
	return groups;
    }
#endif

    const int currentGroupLevel(const Opm::Schedule& sched, const Opm::Group& group, const size_t simStep)
    {
	int level = 0;
	const std::vector< const Opm::Group* >  groups = sched.getGroups(simStep);
      	const std::string& groupName = group.name();
	if (!sched.hasGroup(groupName))
            throw std::invalid_argument("No such group: " + groupName);
        {
            if (group.hasBeenDefined( simStep )) {
                const auto& groupTree = sched.getGroupTree( simStep );
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
		throw std::invalid_argument("actual group has not been defined at report time: " + simStep );
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
        void staticContrib(const Opm::Schedule&    sched,
			   const Opm::Group&       group,
			   const int               nwgmax,
			   const std::size_t       simStep,
			   IGrpArray&              igrp,
			   const std::vector<int>& inteHead)
        {
	  // find the number of wells or child groups belonging to a group and store in 
	  // location nwgmax +1 in the igrp array

	    const auto& childGroups = sched.getChildGroups(group.name(), simStep);
	    const auto& childWells  = sched.getChildWells(group.name(), simStep);
	    //std::cout << "IGrpArray - staticContrib - before currentGroupMapIndexGroup(sched, simStep, inteHead)"  <<  std::endl;
	    //const auto& groupMapIndexGroup = currentGroupMapIndexGroup(sched, simStep, inteHead);
	    std::cout << "IGrpArray - staticContrib - before currentGroupMapNameIndex(sched, simStep, inteHead)"  <<  std::endl;
	    const auto& groupMapNameIndex = currentGroupMapNameIndex(sched, simStep, inteHead);
	    if ((childGroups.size() != 0) && (childWells.size()!=0))
	      throw std::invalid_argument("group has both wells and child groups" + group.name());
            int igrpCount = 0;
	    if (childWells.size() != 0) {
		//group has child wells
		for (const auto* well : childWells ) {
		    igrp[igrpCount] = well->seqIndex()+1;
		    igrpCount+=1;
		}
	    }
	    else if (childGroups.size() != 0) {
		//group has child groups
		//The field group always has seqIndex = 0 because it is always defined first
	        //Hence the all groups except the Field group uses the seqIndex assigned
		for (const auto& group : childGroups ) {
		    igrp[igrpCount] = group->seqIndex()+1;
		    igrpCount+=1;
		}
	    }

	    //assign the number of child wells or child groups to location
	    // nwgmax
	    igrp[nwgmax] =  (childGroups.size() == 0)
                    ? childWells.size() : childGroups.size();

	    // find the group type (well group (type 0) or node group (type 1) and store the type in  
	    // location nwgmax + 26
	    const auto grpType = groupType(sched, group, simStep);
	    igrp[nwgmax+26] = grpType;

	    //find group level ("FIELD" is level 0) and store the level in
	    //location nwgmax + 27
	    const auto grpLevel = currentGroupLevel(sched, group, simStep);
	    igrp[nwgmax+27] = grpLevel;

	    //find parent group and store index of parent group in
	    //location nwgmax + 28
	    const auto& groupTree = sched.getGroupTree( simStep );
	    const std::string& parent = groupTree.parent(group.name());
	    std::cout << "IGrpArray - staticContrib - before groupMapNameIndex.find(parent)"  <<  std::endl;
	    if (group.name() == "FIELD")
	      igrp[nwgmax+28] = 0;
	    else {
		if (parent.size() == 0)
		    throw std::invalid_argument("parent group does not exist" + group.name());
		const auto itr = groupMapNameIndex.find(parent);
		igrp[nwgmax+28] = (itr->second)+1;
	    }
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
captureDeclaredGroupData(const Schedule&         sched,
                         const std::size_t       simStep,
			 const std::vector<int>& inteHead)
{
    std::cout << "captureDeclaredGroupData before creation groups"  << std::endl;
    //const auto& groups = sched.getGroups(simStep);
    std::map <size_t, const Opm::Group*> indexGroupMap = currentGroupMapIndexGroup(sched, simStep, inteHead);
    std::cout << "captureDeclaredGroupData after creation groups"  << std::endl;
    std::vector<const Opm::Group*> curGroups;
    curGroups.reserve(ngmaxz(inteHead)+1);

    std::map <size_t, const Opm::Group*>::iterator it = indexGroupMap.begin();
    while (it != indexGroupMap.end()) {
        std::cout << "captureDeclaredGroupData before assign curGroups, it-first: " << it->first << " it-> second->name(): " << it->second->name() << std::endl;
        curGroups[static_cast<int>(it->first)] = it->second;
        it++;
    }

    //
    std::cout << "captureDeclaredGroupData curGroups.size(): " << curGroups.size() << std::endl;

    std::cout << "curGroups -index: " << 0  << " names: " << curGroups[0]->name() << std::endl;

    //std::cout << "curGroups -index: " << i  << " names: " << curGroups[i]->name() << std::endl;
    for (const auto* grp : curGroups)
    {
	std::cout << "curGroups grp " << grp->name() << std::endl;
    }

    {
	std::cout << "captureDeclaredGroupData before loop groupLoop"  << std::endl;
        groupLoop(curGroups, [sched, simStep, inteHead, this]
            (const Group& group, const std::size_t groupID) -> void
        {
            auto ig = this->iGroup_[groupID];
	    std::cout << "captureDeclaredGroupData: groupLoop, groupID: "  <<  groupID << std::endl;
            IGrp::staticContrib(sched, group, this->nGMaxz_, simStep, ig, inteHead);
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
                       const std::size_t             simStep,
                       const ::Opm::data::WellRates& wRates)
{
    const auto& wells = sched.getWells(simStep);

    wellLoop(wells, [this, &phases, &wRates]
        (const Well& well, const std::size_t wellID) -> void
    {
        auto xw = this->xWell_[wellID];

        XWell::dynamicContrib(well, phases, wRates, xw);
    });
}
#endif
