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

#include <opm/output/eclipse/AggregateMSWData.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellConnections.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

//#include <opm/output/data/Wells.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <iostream>
#include <limits>

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateMSWData
// ---------------------------------------------------------------------

namespace {

    std::size_t nummsw(const std::vector<int>& inteHead)
    {
        // inteHead(174) = NSEGWL
        return inteHead[174];
    }

    std::size_t nswlmx(const std::vector<int>& inteHead)
    {
        // inteHead(175) = NSWLMX
        return inteHead[175];
    }

    std::size_t nisegz(const std::vector<int>& inteHead)
    {
        // inteHead(178) = NISEGZ
        return inteHead[178];
    }

    std::size_t nrsegz(const std::vector<int>& inteHead)
    {
        // inteHead(179) = NRSEGZ
        return inteHead[179];
    }

    std::size_t nilbrz(const std::vector<int>& inteHead)
    {
        // inteHead(180) = NILBRZ
        return inteHead[180];
    }

    Opm::RestartIO::Helpers::BranchSegmentPar
    getBranchSegmentParam(const Opm::WellSegments& segSet, const int branch)
    {
	int noSegInBranch = 0;
	int firstSeg = -1;
	int lastSeg  = -1;
	int outletS = 0;
	for (size_t segNo = 1; segNo <= segSet.size(); segNo++) {
	    auto segInd = segSet.segmentNumberToIndex(segNo);
	    auto i_branch = segSet[segInd].branchNumber();
	    auto i_outS = segSet[segInd].outletSegment();
	    if (i_branch == branch) {
		noSegInBranch +=1;
		if (firstSeg < 0) {
		    firstSeg = segNo;
		    outletS = (branch > 1) ? i_outS : 0;
		}
		lastSeg = segNo;
	    }
	}

	return {
	  outletS,
	  noSegInBranch,
	  firstSeg,
	  lastSeg,
	  branch
	};
    }

    std::vector<size_t> SegmentSetBranches(const Opm::WellSegments& segSet) {
	std::vector<size_t> branches;
	for (size_t segNo = 1; segNo <= segSet.size(); segNo++) {
	    auto segInd = segSet.segmentNumberToIndex(segNo);
	    auto i_branch = segSet[segInd].branchNumber();
	    if (std::find(branches.begin(), branches.end(), i_branch) == branches.end()) { 
		branches.push_back(i_branch);
	    }
	}
	return branches;
    }

    int firstSegmentInBranch(const Opm::WellSegments& segSet, const int branch) {
	int firstSegNo = 0;
	size_t segNo = 0;
	while ((segNo <= segSet.size()) && (firstSegNo == 0)) {
	    segNo+=1;
	    auto segInd = segSet.segmentNumberToIndex(segNo);
	    auto i_branch = segSet[segInd].branchNumber();
	    if (branch == i_branch) {
		firstSegNo = segNo;
	    }
	}
	return firstSegNo;
    }

     int noConnectionsSegment(const Opm::WellConnections& compSet,
                             const Opm::WellSegments&    segSet,
                             const size_t              segIndex)
    {
	auto segNumber  = segSet[segIndex].segmentNumber();
	int noConnections = 0;
	for (auto it : compSet) {
	    auto cSegment = it.segment_number;
	    if (segNumber == cSegment) {
		noConnections+=1;
	    }
	}

	return noConnections;
    }
    
    int sumConnectionsSegment(const Opm::WellConnections& compSet,
                                 const Opm::WellSegments&    segSet,
                                 const size_t              segIndex)
    {
      // This function returns (for a given segment) the sum of number of connections for each segment
      // with lower segment index than the currnet segment
      // If the segment contains no connections, the number returned is zero.
	int sumConn = 0;
	if (noConnectionsSegment(compSet, segSet, segIndex) > 0) {
	// add up the number of connections for å segments with lower segment index than current segment
	    auto segNumber  = segSet[segIndex].segmentNumber();
	    for (std::size_t i_seg = 1; i_seg <= segNumber; i_seg++) {
		auto  ind = segSet.segmentNumberToIndex(i_seg);
		sumConn += noConnectionsSegment(compSet, segSet, ind);
	    }
	}
	return sumConn;
    }

    
    /*int firstConnectionInSegment(const Opm::WellConnections& compSet,
                                 const Opm::WellSegments&    segSet,
                                 const size_t              segIndex,
				 const Opm::EclipseGrid&  grid)
    {
	auto segNumber  = segSet[segIndex].segmentNumber();
	int firstConnection = std::numeric_limits<int>::max();
	for (auto it : compSet) {
	    auto c_Segment  = it.segment_number;
	    auto c_SeqIndex = it.getSeqIndex();
	    const auto c_active = grid.cellActive(it.getI(), it.getJ(), it.getK());
	    if ((segNumber == c_Segment) && (c_SeqIndex < firstConnection) && c_active) {
		firstConnection = c_SeqIndex;
	    }
	}
	return (firstConnection == std::numeric_limits<int>::max()) ? 0: firstConnection+1;
    }*/



    std::vector<size_t>
    inflowSegmentsIndex(const Opm::WellSegments& segSet, size_t segIndex) {
	auto segNumber  = segSet[segIndex].segmentNumber();
	std::vector<size_t> inFlowSegNum;
	for (size_t ind = 0; ind < segSet.size(); ind++) {
	    auto i_outletSeg = segSet[ind].outletSegment();
	    if (segNumber == i_outletSeg) {
		inFlowSegNum.push_back(ind);
	    }
	}
	return inFlowSegNum;
    }

    int noInFlowBranches(const Opm::WellSegments& segSet, size_t segIndex) {
	auto segNumber  = segSet[segIndex].segmentNumber();
	auto branch     = segSet[segIndex].branchNumber();
	int noIFBr = 0;
	for (size_t ind = 0; ind < segSet.size(); ind++) {
	    auto o_segNum = segSet[ind].outletSegment();
	    auto i_branch = segSet[ind].branchNumber();
	    if ((segNumber == o_segNum) && (branch != i_branch)){
		noIFBr+=1;
	    }
	}
	return noIFBr;
    }
    //find the number of inflow branches (different from the current branch)
    int sumNoInFlowBranches(const Opm::WellSegments& segSet, size_t segIndex) {
	int sumIFB = 0;
	auto segBranch     = segSet[segIndex].branchNumber();
	const auto iSInd = inflowSegmentsIndex(segSet, segIndex);
	for (auto ind : iSInd) {
	    auto inflowBranch = segSet[ind].branchNumber();
	    // if inflow segment belongs to different branch add contribution
	    if (segBranch != inflowBranch) {
		sumIFB+=1;
		// search recursively down this branch to find more inflow branches
		sumIFB += sumNoInFlowBranches(segSet, ind);
	    }
	    /*else {
		// search down the branch to find more inflow branches
		sumIFB += sumNoInFlowBranches(segSet, ind);
	    }*/
	}
	return sumIFB;
    }
    
    /*int sumNoInFlowBranches(const Opm::WellSegments& segSet, size_t segIndex) {
	int sumIFB = 0;
	size_t indS = segIndex;
	bool nonSeg = false;
	while ((indS >= 0) && (!nonSeg)) {
	    auto segNumber  = segSet[indS].segmentNumber();
	    auto branch     = segSet[indS].branchNumber();
	    auto outletS    = segSet[indS].outletSegment();
	    sumIFB += noInFlowBranches(segSet, indS);
	    indS = segSet.segmentNumberToIndex(outletS);
	    nonSeg = (segNumber == 1) ? true : false;
	}
	return sumIFB;
    }*/

     std::vector<std::size_t> segmentOrder(const Opm::WellSegments& segSet, const std::size_t segIndex) {
	//auto branches =  SegmentSetBranches(segSet);
	//std::size_t noBranches = branches.size();
	//std::size_t noSegments = segSet.size();
	//std::cout << "segmentOrder - noBranches: " << noBranches << std::endl;
	//std::cout << "segmentOrder - noSegments: " << noSegments << std::endl;
	std::vector<size_t> ordSegNumber;
	std::vector<size_t> tempOrdVect;
	std::vector<size_t> segIndCB;
	// Store "heel" segment since that will not always be at the end of the list
	segIndCB.push_back(segIndex);
	std::size_t sInd = segIndex;
	std::size_t newSInd = segIndex;
	auto origBranchNo = segSet[segIndex].branchNumber();
	// loop down branch to find all segments in branch and number from "toe" to "heel"
	std::cout << "segmentOrder - segIndex: " << segIndex  << std::endl;
	while (newSInd < segSet.size()) {
	    //if (segSet[newSInd].branchNumber() == origBranchNo) {
		const auto iSInd = inflowSegmentsIndex(segSet, newSInd);
		if (iSInd.size() > 0) {
		    for (auto ind : iSInd) {
			auto inflowBranch = segSet[ind].branchNumber();
			if (origBranchNo == inflowBranch) {
			    // if inflow segment belongs to same branch add contribution
			    segIndCB.insert(segIndCB.begin(), ind);
			    // search recursively down this branch to find more inflow branches
			    newSInd = ind;
			    std::cout << "segmentOrder - origBranch - inflowBranch: " << inflowBranch << "  ind: " << ind  << std::endl;
			}
			else {
			    // if inflow segment belongs to different branch, start new search
			    std::cout << "segmentOrder - newBranch - inflowBranch: " << inflowBranch << std::endl;
			    auto nSOrd = segmentOrder(segSet, ind);
			    // copy the segments already found and indexed into the total ordered segment vector
			    ordSegNumber.resize(nSOrd.size(),0);
			    for (std::size_t indOS = 0; indOS < nSOrd.size(); indOS++) {
				ordSegNumber[indOS] = nSOrd[indOS];
				std::cout << "segmentOrder - newBranch - ordSegNumber[indOS] " << indOS << "  nSOrd: " <<  nSOrd[indOS] << std::endl;
			    }
			    // increment the local branch sequence number counter
			}
		    }
		}
		else {
		    // have come to toe of current branch - store segment indicies of current branch
		    std::cout << "segmentOrder -  store segments origBranchNo " <<  origBranchNo << std::endl;
		    for (std::size_t indOS = 0; indOS < segIndCB.size(); indOS++) {
			ordSegNumber.push_back(segIndCB[indOS]);
			std::cout << "segmentOrder - origBranchNo- indOS " << indOS << "  segIndCB: " <<  segIndCB[indOS] << std::endl;
		    }
		    // set new index to exit while loop
		    newSInd = segSet.size();
		}
	    //}
	}

	std::cout << "end section segmentOrder - origBranchNo: " << origBranchNo << std::endl;
	if (origBranchNo == 1) {
	    // make the vector of ordered segments
	    tempOrdVect.resize(ordSegNumber.size());
	    for (std::size_t ov_ind = 0; ov_ind < ordSegNumber.size(); ov_ind++) {
		tempOrdVect[ordSegNumber[ov_ind]] = ov_ind+1;
		std::cout << "segmentOrder - Reorder segments - ov_ind " << ov_ind << "  tempOrdVect[ordSegNumber[ov_ind]] " <<  tempOrdVect[ordSegNumber[ov_ind]] << std::endl;
	    }
	    return tempOrdVect;
	} else {
	    return ordSegNumber;
	}
    }

        /*int noUpstreamSeg(const Opm::WellSegments& segSet, size_t segIndex) {
	auto branch = segSet[segIndex].branchNumber();
	int noUpstrSeg  = 1;
	size_t ind = segIndex;
	size_t indMainBranch = segIndex;
	size_t oldIndMainBranch = segSet.size()+1;
	while (ind < segSet.size() && (indMainBranch != oldIndMainBranch)) {
	    oldIndMainBranch = indMainBranch;
	    auto inFlowSegsInd = inflowSegmentsIndex(segSet, ind);
	    if (inFlowSegsInd.size() > 0) {
		for (auto segI : inFlowSegsInd) {
		    auto u_segNum = segSet[segI].segmentNumber();
		    auto u_branch = segSet[segI].branchNumber();
		    if (branch == u_branch) {
			// upstream segment is on same branch - continue search on same branch
			noUpstrSeg +=1;
			indMainBranch = segI;
		    }
		    if (branch != u_branch)  {
			// upstream segment is on other branch - search along this branch to end of branch
			noUpstrSeg +=1;
			int inc_noUpstrSeg = noUpstreamSeg(segSet, segI);
			noUpstrSeg += inc_noUpstrSeg;
		    }
		}
	    }
	    else {
		indMainBranch = segSet.size();
	    }
	ind = indMainBranch;
	}
	return noUpstrSeg;
    }*/
    
    
    
    int inflowSegmentCurBranch(const Opm::WellSegments& segSet, size_t segIndex) {
	auto branch = segSet[segIndex].branchNumber();
	auto segNumber  = segSet[segIndex].segmentNumber();
	int inFlowSegNum = 0;
	for (size_t ind = 0; ind < segSet.size(); ind++) {
	    auto i_segNum = segSet[ind].segmentNumber();
	    auto i_branch = segSet[ind].branchNumber();
	    auto i_outFlowSeg = segSet[ind].outletSegment();
	    if ((branch == i_branch) && (segNumber == i_outFlowSeg)) {
		if (inFlowSegNum == 0) {
		    inFlowSegNum = i_segNum;
		}
		else {
		  std::cout << "Non-unique inflow segment in same branch, Well: " << segSet.wellName() << std::endl;
		  std::cout <<  "Segment number: " << segNumber << std::endl;
		  std::cout <<  "Branch number: " << branch << std::endl;
		  std::cout <<  "Inflow segment number 1: " << inFlowSegNum << std::endl;
		  std::cout <<  "Inflow segment number 2: " << segSet[ind].segmentNumber() << std::endl;
		  throw std::invalid_argument("Non-unique inflow segment in same branch, Well " + segSet.wellName());
		}
	    }
	}
	return inFlowSegNum;
    }

    template <typename MSWOp>
    void MSWLoop(const std::vector<const Opm::Well*>& wells,
                  MSWOp&&                             mswOp)
    {
        auto mswID = std::size_t{0};
        for (const auto* well : wells) {
            mswID += 1;

            if (well == nullptr) { continue; }

            mswOp(*well, mswID - 1);
        }
    }

    namespace ISeg {
        std::size_t entriesPerMSW(const std::vector<int>& inteHead)
        {
            // inteHead(176) = NSEGMX
            // inteHead(178) = NISEGZ
            return inteHead[176] * inteHead[178];
        }

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

            return WV {
                WV::NumWindows{ nswlmx(inteHead) },
                WV::WindowSize{ entriesPerMSW(inteHead) }
            };
        }

        template <class ISegArray>
        void staticContrib(const Opm::Well&        well,
                           const std::size_t       rptStep,
                           const std::vector<int>& inteHead,
			   const Opm::EclipseGrid&  grid,
                           ISegArray&              iSeg
			  )
        {
            if (well.isMultiSegment(rptStep)) {
		//loop over segment set and print out information
		auto welSegSet = well.getWellSegments(rptStep);
		auto completionSet = well.getConnections(rptStep);
		auto noElmSeg = nisegz(inteHead);
		std::size_t segmentInd = 0;
		auto orderedSegmentNo = segmentOrder(welSegSet, segmentInd);
		std::cout << "Ordered segment index: " << std::endl;
		for (std::size_t loopInd = 0; loopInd < orderedSegmentNo.size(); loopInd++) {
		    std::cout << "loopInd: " << loopInd << "  ordSegNo: " << orderedSegmentNo[loopInd] << std::endl;
		}
		for (size_t ind_seg = 1; ind_seg <= welSegSet.size(); ind_seg++) {
		    auto ind = welSegSet.segmentNumberToIndex(ind_seg);
		    auto iS = (ind_seg-1)*noElmSeg;
		    iSeg[iS + 0] = orderedSegmentNo[ind];
		    iSeg[iS + 1] = welSegSet[ind].outletSegment();
		    iSeg[iS + 2] = inflowSegmentCurBranch(welSegSet, ind);
		    iSeg[iS + 3] = welSegSet[ind].branchNumber();
		    iSeg[iS + 4] = noInFlowBranches(welSegSet, ind);
		    iSeg[iS + 5] = sumNoInFlowBranches(welSegSet, ind);
		    iSeg[iS + 6] = noConnectionsSegment(completionSet, welSegSet, ind);
		    iSeg[iS + 7] = sumConnectionsSegment(completionSet, welSegSet, ind);
		    iSeg[iS + 8] = iSeg[iS+0];
		}
	    }
	    else {
	      throw std::invalid_argument("No such multisegment well: " + well.name());
	    }
        }
    } // ISeg

    namespace RSeg {
        std::size_t entriesPerMSW(const std::vector<int>& inteHead)
        {
            // inteHead(176) = NSEGMX
            // inteHead(179) = NRSEGZ
            return inteHead[176] * inteHead[179];
        }

        Opm::RestartIO::Helpers::WindowedArray<double>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<double>;

            return WV {
                WV::NumWindows{ nswlmx(inteHead) },
                WV::WindowSize{ entriesPerMSW(inteHead) }
            };
        }

        template <class RSegArray>
        void staticContrib(const Opm::Well&        well,
                           const std::size_t       rptStep,
                           const std::vector<int>& inteHead,
			   const Opm::UnitSystem& units,
                           RSegArray&              rSeg)
        {
	    if (well.isMultiSegment(rptStep)) {
		using M = ::Opm::UnitSystem::measure;
		//loop over segment set and print out information
		auto welSegSet = well.getWellSegments(rptStep);
		auto completionSet = well.getCompletions(rptStep);
		auto noElmSeg = nrsegz(inteHead);
		    //treat the top segment individually
		    int ind_seg = 1;
		    auto ind = welSegSet.segmentNumberToIndex(ind_seg);
		    rSeg[0] = units.from_si(M::length, welSegSet.lengthTopSegment());
		    rSeg[1] = units.from_si(M::length, welSegSet.depthTopSegment());
		    rSeg[5] = units.from_si(M::volume, welSegSet.volumeTopSegment());
		    rSeg[6] = rSeg[0];
		    rSeg[7] = rSeg[1];

		    //  segment pressure (to be added!!)
		    rSeg[ 39] = 0;

		    //Default values
		    rSeg[ 39] = 1.0;

		    rSeg[105] = 1.0;
		    rSeg[106] = 1.0;
		    rSeg[107] = 1.0;
		    rSeg[108] = 1.0;
		    rSeg[109] = 1.0;
		    rSeg[110] = 1.0;

		//Treat subsequent segments
		for (size_t ind_seg = 2; ind_seg <= welSegSet.size(); ind_seg++) {
		    // set the elements of the rSeg array
		    auto ind = welSegSet.segmentNumberToIndex(ind_seg);
		    auto outSeg = welSegSet[ind].outletSegment();
		    auto ind_ofs = welSegSet.segmentNumberToIndex(outSeg);
		    auto iS = (ind_seg-1)*noElmSeg;
		    rSeg[iS +   0] = units.from_si(M::length, (welSegSet[ind].totalLength() - welSegSet[ind_ofs].totalLength()));
		    rSeg[iS +   1] = units.from_si(M::length, (welSegSet[ind].depth() - welSegSet[ind_ofs].depth()));
		    rSeg[iS +   2] = units.from_si(M::length, (welSegSet[ind].internalDiameter()));
		    rSeg[iS +   3] = units.from_si(M::length, (welSegSet[ind].roughness()));
		    rSeg[iS +   4] = units.from_si(M::length, (welSegSet[ind].crossArea()));
		    //repeat unit conversion to convert for area instead of length
		    rSeg[iS +   4] = units.from_si(M::length, rSeg[iS +   4]);
		    rSeg[iS +   5] = units.from_si(M::volume, (welSegSet[ind].volume()));
		    rSeg[iS +   6] = units.from_si(M::length, (welSegSet[ind].totalLength()));
		    rSeg[iS +   7] = units.from_si(M::length, (welSegSet[ind].depth()));

		    //  segment pressure (to be added!!)
		    rSeg[iS +  11] = 0;

		    //Default values
		    rSeg[iS +  39] = 1.0;

		    rSeg[iS + 105] = 1.0;
		    rSeg[iS + 106] = 1.0;
		    rSeg[iS + 107] = 1.0;
		    rSeg[iS + 108] = 1.0;
		    rSeg[iS + 109] = 1.0;
		    rSeg[iS + 110] = 1.0;
		}
	    }
	    else {
	      throw std::invalid_argument("No such multisegment well: " + well.name());
	    }
        }
    } // RSeg

    namespace ILBS {
        std::size_t entriesPerMSW(const std::vector<int>& inteHead)
        {
            // inteHead(177) = NLBRMX
            return inteHead[177];
        }

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

            return WV {
                WV::NumWindows{ nswlmx(inteHead) },
                WV::WindowSize{ entriesPerMSW(inteHead) }
            };
        }

        template <class ILBSArray>
        void staticContrib(const Opm::Well&  well,
                           const std::size_t rptStep,
                           ILBSArray&        iLBS)
        {
	    if (well.isMultiSegment(rptStep)) {
		//
		auto welSegSet = well.getWellSegments(rptStep);
		auto branches = SegmentSetBranches(welSegSet);
		for (auto it = branches.begin()+1; it != branches.end(); it++){
		    iLBS[*it-2] = firstSegmentInBranch(welSegSet, *it);
		}
	    }
	    else {
	      throw std::invalid_argument("No such multisegment well: " + well.name());
	    }
        }
    } // ILBS

    namespace ILBR {
        std::size_t entriesPerMSW(const std::vector<int>& inteHead)
        {
            // inteHead(177) = NLBRMX
            // inteHead(180) = NILBRZ
            return inteHead[177] * inteHead[180];
        }

        Opm::RestartIO::Helpers::WindowedArray<int>
        allocate(const std::vector<int>& inteHead)
        {
            using WV = Opm::RestartIO::Helpers::WindowedArray<int>;

            return WV {
                WV::NumWindows{ nswlmx(inteHead) },
                WV::WindowSize{ entriesPerMSW(inteHead) }
            };
        }

        template <class ILBRArray>
        void staticContrib(const Opm::Well&  well,
                           const std::size_t rptStep,
			   const std::vector<int>& inteHead,
                           ILBRArray&        iLBR)
        {
	    if (well.isMultiSegment(rptStep)) {
		//
		auto welSegSet = well.getWellSegments(rptStep);
		auto branches = SegmentSetBranches(welSegSet);
		auto noElmBranch = nilbrz(inteHead);
		for (auto it = branches.begin(); it != branches.end(); it++){
		    auto iB = (*it-1)*noElmBranch;
		    auto branchParam = getBranchSegmentParam(welSegSet, *it);
		    iLBR[iB  ] = branchParam.outletS;
		    iLBR[iB+1] = branchParam.noSegInBranch;
		    iLBR[iB+2] = branchParam.firstSeg;
		    iLBR[iB+3] = branchParam.lastSeg;
		    iLBR[iB+4] = branchParam.branch - 1;
		}
	    }
	    else {
	      throw std::invalid_argument("No such multisegment well: " + well.name());
	    }
        }
    } // ILBR

} // Anonymous

// =====================================================================

Opm::RestartIO::Helpers::AggregateMSWData::
AggregateMSWData(const std::vector<int>& inteHead)
    : iSeg_ (ISeg::allocate(inteHead))
    , rSeg_ (RSeg::allocate(inteHead))
    , iLBS_ (ILBS::allocate(inteHead))
    , iLBR_ (ILBR::allocate(inteHead))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateMSWData::
captureDeclaredMSWData(const Schedule&         sched,
                       const std::size_t       rptStep,
		       const Opm::UnitSystem& units,
                       const std::vector<int>& inteHead,
		       const Opm::EclipseGrid&  grid)
{
    const auto& wells = sched.getWells(rptStep);
    auto msw = std::vector<const Opm::Well*>{};

    //msw.reserve(wells.size());
    for (const auto well : wells) {
	if (well->isMultiSegment(rptStep)) msw.push_back(well);
    }

    // Extract Contributions to ISeg Array
    {
        MSWLoop(msw, [rptStep, inteHead, grid, this]
            (const Well& well, const std::size_t mswID) -> void
        {
            auto imsw = this->iSeg_[mswID];

            ISeg::staticContrib(well, rptStep, inteHead, grid, imsw);
        });
    }

    // Extract Contributions to RSeg Array
    {
        MSWLoop(msw, [&units, rptStep, inteHead, this]
            (const Well& well, const std::size_t mswID) -> void
        {
            auto rmsw = this->rSeg_[mswID];

            RSeg::staticContrib(well, rptStep, inteHead, units, rmsw);
        });
    }

    // Extract Contributions to ILBS Array
    {
        MSWLoop(msw, [rptStep, this]
            (const Well& well, const std::size_t mswID) -> void
        {
            auto ilbs_msw = this->iLBS_[mswID];

            ILBS::staticContrib(well, rptStep, ilbs_msw);
        });
    }

    // Extract Contributions to ILBR Array
    {
        MSWLoop(msw, [rptStep, inteHead, this]
            (const Well& well, const std::size_t mswID) -> void
        {
            auto ilbr_msw = this->iLBR_[mswID];

            ILBR::staticContrib(well, rptStep, inteHead, ilbr_msw);
        });
    }
}
