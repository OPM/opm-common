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

#include <opm/output/eclipse/SummaryState.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
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
	std::map <size_t, const Opm::Group*> indexGroupMap;
	for (const auto* group : groups) {
	    int ind = (group->name() == "FIELD")
	    ? ngmaxz(inteHead)-1 : group->seqIndex()-1;
	    const std::pair<size_t, const Opm::Group*> groupPair = std::make_pair(static_cast<size_t>(ind), group); 
	    indexGroupMap.insert(groupPair);
	}
	return indexGroupMap;
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

#if 0
    namespace SWell {
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
        template <class ZWellArray>
        void staticContrib(const Opm::Well& well, ZWellArray& zWell)
        {
            zWell[1 - 1] = well.name();
        }
    } // ZWell
#endif

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
			   const int               ngmaxz,
			   const std::size_t       simStep,
			   IGrpArray&              iGrp,
			   const std::vector<int>& inteHead)
        {
	  // find the number of wells or child groups belonging to a group and store in 
	  // location nwgmax +1 in the iGrp array

	    const auto childGroups = sched.getChildGroups(group.name(), simStep);
	    const auto childWells  = sched.getChildWells(group.name(), simStep);
	    //std::cout << "IGrpArray - staticContrib - before currentGroupMapIndexGroup(sched, simStep, inteHead)"  <<  std::endl;
	    //const auto& groupMapIndexGroup = currentGroupMapIndexGroup(sched, simStep, inteHead);
	    //std::cout << "IGrpArray - staticContrib - before currentGroupMapNameIndex(sched, simStep, inteHead)"  <<  std::endl;
	    const auto groupMapNameIndex = currentGroupMapNameIndex(sched, simStep, inteHead);
	    if ((childGroups.size() != 0) && (childWells.size()!=0))
	      throw std::invalid_argument("group has both wells and child groups" + group.name());
            int igrpCount = 0;
	    if (childWells.size() != 0) {
		//group has child wells
		//std::cout << "IGrpArray - staticContrib: childwells for group.name(): " << group.name()  << "childWells - size: " << childWells.size() << std::endl;
		//for (const auto well : childWells ) {
		for ( auto it = childWells.begin() ; it != childWells.end(); it++) {
		    //std::cout << "Child well name: " << it->name() << " Well seqIndex(): " << it->seqIndex() << std::endl;
		    iGrp[igrpCount] = (*it)->seqIndex()+1;
		    igrpCount+=1;
		    //std::cout << "childWells: igrpCount after increment: " << igrpCount <<  std::endl;
		}
	    }
	    else if (childGroups.size() != 0) {
		//group has child groups
		//The field group always has seqIndex = 0 because it is always defined first
	        //Hence the all groups except the Field group uses the seqIndex assigned
		//std::cout << "IGrpArray - staticContrib: childGroups for group.name(): " << group.name()  << "childGroups - size: " << childGroups.size() << std::endl;
		//for (const auto grp : childGroups ) {
		for ( auto it = childGroups.begin() ; it != childGroups.end(); it++) {
		    //std::cout << "Child Group name: " << it->name() << " Group seqIndex(): " << it->seqIndex()-1 << std::endl;
		    iGrp[igrpCount] = (*it)->seqIndex();
		    igrpCount+=1;
		    //std::cout << "childGroups: igrpCount after increment: " << igrpCount <<  std::endl;
		}
	    }

	    //assign the number of child wells or child groups to
	    // location nwgmax
	    //std::cout << "IGrpArray - staticContrib: childGroups.size() " << childGroups.size()  << "childWells.size(): " << childWells.size() << std::endl;
	    //std::cout << "IGrpArray - staticContrib: nwgmax: " << nwgmax  << std::endl;
	    iGrp[nwgmax] =  (childGroups.size() == 0)
                    ? childWells.size() : childGroups.size();

	    // find the group type (well group (type 0) or node group (type 1) and store the type in  
	    // location nwgmax + 26
	    const auto grpType = groupType(sched, group, simStep);
	    iGrp[nwgmax+26] = grpType;

	    //find group level ("FIELD" is level 0) and store the level in
	    //location nwgmax + 27
	    const auto grpLevel = currentGroupLevel(sched, group, simStep);

	    // set values for group probably connected to GCONPROD settings
	    //
	    if (group.name() != "FIELD")
	    {
		iGrp[nwgmax+ 5] = -1;
		iGrp[nwgmax+12] = -1;
		iGrp[nwgmax+17] = -1;
		iGrp[nwgmax+22] = -1;

		//assign values to group number (according to group sequence)
		iGrp[nwgmax+88] = group.seqIndex();
		iGrp[nwgmax+89] = group.seqIndex();
		iGrp[nwgmax+95] = group.seqIndex();
		iGrp[nwgmax+96] = group.seqIndex();
	    }
	    else
	    {
		//assign values to group number (according to group sequence)
		iGrp[nwgmax+88] = ngmaxz;
		iGrp[nwgmax+89] = ngmaxz;
		iGrp[nwgmax+95] = ngmaxz;
		iGrp[nwgmax+96] = ngmaxz;
	    }
	    //find parent group and store index of parent group in
	    //location nwgmax + 28
	    const auto& groupTree = sched.getGroupTree( simStep );
	    const std::string& parent = groupTree.parent(group.name());
	    //std::cout << "IGrpArray - staticContrib - before groupMapNameIndex.find(parent)"  <<  std::endl;
	    if (group.name() == "FIELD")
	      iGrp[nwgmax+28] = 0;
	    else {
		if (parent.size() == 0)
		    throw std::invalid_argument("parent group does not exist" + group.name());
		const auto itr = groupMapNameIndex.find(parent);
		iGrp[nwgmax+28] = (itr->second)+1;
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

	template <class SGrpArray>
	void staticContrib(SGrpArray& sGrp)
	{
		const auto dflt   = -1.0e+20f;
		const auto dflt_2 = -2.0e+20f;
		const auto infty  =  1.0e+20f;
		const auto zero   =  0.0f;
		const auto one    =  1.0f;

		const auto init = std::vector<float> { // 132 Items (0..131)
		    // 0     1      2      3      4
		    infty, infty, dflt , infty,  zero ,     //   0..  4  ( 0)
		    zero , infty, infty, infty , infty,     //   5..  9  ( 1)
		    infty, infty, infty, infty , dflt ,     //  10.. 14  ( 2)
		    infty, infty, infty, infty , dflt ,     //  15.. 19  ( 3)
		    infty, infty, infty, infty , dflt ,     //  20.. 24  ( 4)
		    zero , zero , zero , dflt_2, zero ,     //  24.. 29  ( 5)
		    zero , zero , zero , zero  , zero ,     //  30.. 34  ( 6)
		    infty ,zero , zero , zero  , infty,     //  35.. 39  ( 7)
		    zero , zero , zero , zero  , zero ,     //  40.. 44  ( 8)
		    zero , zero , zero , zero  , zero ,     //  45.. 49  ( 9)
		    zero , infty, infty, infty , infty,     //  50.. 54  (10)
		    infty, infty, infty, infty , infty,     //  55.. 59  (11)
		    infty, infty, infty, infty , infty,     //  60.. 64  (12)
		    infty, infty, infty, infty , zero ,     //  65.. 69  (13)
		    zero , zero , zero , zero  , zero ,     //  70.. 74  (14)
		    zero , zero , zero , zero  , infty,     //  75.. 79  (15)
		    infty, zero , infty, zero  , zero ,     //  80.. 84  (16)
		    zero , zero , zero , zero  , zero ,     //  85.. 89  (17)
		    zero , zero , one  , zero  , zero ,     //  90.. 94  (18)
		    zero , zero , zero , zero  , zero ,     //  95.. 99  (19)
		    zero , zero , zero , zero  , zero ,     // 100..104  (20)
		    zero , zero , zero , zero  , zero ,     // 105..109  (21)
		    zero , zero                             // 110..111  (22)
		};

		const auto sz = static_cast<
		    decltype(init.size())>(sGrp.size());

		auto b = std::begin(init);
		auto e = b + std::min(init.size(), sz);

		std::copy(b, e, std::begin(sGrp));
	}
    } // SGrp

    namespace XGrp {
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

        // here define the dynamic group quantities to be written to the restart file
        template <class XGrpArray>
        void dynamicContrib(const std::vector<std::string>&      restart_group_keys,
			    const std::vector<std::string>&      restart_field_keys,
			    const std::map<std::string, size_t>& groupKeyToIndex,
			    const std::map<std::string, size_t>& fieldKeyToIndex,
			    const Opm::Group&                    group,
			    const Opm::SummaryState&             sumState,
			    XGrpArray&                           xGrp)
        {
	  std::string groupName = group.name();
	  const std::vector<std::string>& keys = (groupName == "FIELD")
	  ? restart_field_keys : restart_group_keys;
	  const std::map<std::string, size_t>& keyToIndex = (groupName == "FIELD")
	  ? fieldKeyToIndex : groupKeyToIndex;
	  for (const auto key : keys) {
	      std::string compKey = (groupName == "FIELD")
	      ? key : key + ":" + groupName;
	      std::cout << "AggregateGroupData compKey: " << compKey << std::endl;
	      if (sumState.has(compKey)) {
		  double keyValue = sumState.get(compKey);
		  std::cout << "AggregateGroupData_compkey: " << compKey << std::endl;
		  const auto itr = keyToIndex.find(key);
		  xGrp[itr->second] = keyValue;
	      }
	      else {
		  std::cout << "AggregateGroupData, empty " << std::endl;
		  //throw std::invalid_argument("No such keyword: " + compKey);
	    }
	  }
	}
    }

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
} // Anonymous

// =====================================================================

Opm::RestartIO::Helpers::AggregateGroupData::
AggregateGroupData(const std::vector<int>& inteHead)
    : iGroup_ (IGrp::allocate(inteHead))
    , sGroup_ (SGrp::allocate(inteHead))
    , xGroup_ (XGrp::allocate(inteHead))
    , zGroup_ (ZGrp::allocate(inteHead))
    , nWGMax_ (nwgmax(inteHead))
    , nGMaxz_ (ngmaxz(inteHead))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateGroupData::
captureDeclaredGroupData(const Opm::Schedule&                 sched,
			 const std::vector<std::string>&      restart_group_keys,
			 const std::vector<std::string>&      restart_field_keys,
			 const std::map<std::string, size_t>& groupKeyToIndex,
			 const std::map<std::string, size_t>& fieldKeyToIndex,
			 const std::size_t                    simStep,
			 const Opm::SummaryState&             sumState,
			 const std::vector<int>&              inteHead)
{
    std::map <size_t, const Opm::Group*> indexGroupMap = currentGroupMapIndexGroup(sched, simStep, inteHead);
    std::map <std::string, size_t> nameIndexMap = currentGroupMapNameIndex(sched, simStep, inteHead);
    std::vector<const Opm::Group*> curGroups;
    curGroups.resize(ngmaxz(inteHead));
    //
    //initialize curgroups
    for (auto* cg : curGroups) cg = nullptr;

    std::map <size_t, const Opm::Group*>::iterator it = indexGroupMap.begin();
    while (it != indexGroupMap.end())
	{
	    curGroups[static_cast<int>(it->first)] = it->second;
	    it++;
	}

    for (const auto* grp : curGroups)
    {
	if (grp != nullptr)
	{
	    const std::string gname = grp->name();
	    const auto itr = nameIndexMap.find(gname);
	    const int ind = itr->second;
	}
    }

    {
	groupLoop(curGroups, [sched, simStep, inteHead, this]
            (const Group& group, const std::size_t groupID) -> void
	    {
		auto ig = this->iGroup_[groupID];
		IGrp::staticContrib(sched, group, this->nWGMax_, this->nGMaxz_, simStep, ig, inteHead);
	    });
    }

    // Define Static Contributions to SGrp Array.
    groupLoop(curGroups,
        [this](const Group& group, const std::size_t groupID) -> void
    {
        auto sw = this->sGroup_[groupID];
        SGrp::staticContrib(sw);
    });

    // Define DynamicContributions to XGrp Array.
    groupLoop(curGroups,
        [restart_group_keys, restart_field_keys, groupKeyToIndex, fieldKeyToIndex, sumState, this]
	(const Group& group, const std::size_t groupID) -> void
    {
        auto xg = this->xGroup_[groupID];
        XGrp::dynamicContrib( restart_group_keys, restart_field_keys, groupKeyToIndex, fieldKeyToIndex, group, sumState, xg);
    });

#if 0
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
