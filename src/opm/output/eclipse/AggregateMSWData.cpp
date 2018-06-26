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
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>

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

     /*int firstSegmentInBranch(const Opm::SegmentSet& segSet, const int branch) {
	int segNum;
	for (size_t segNo = 1; segNo <= segSet.numberSegment(); segNo++) {
	    auto segInd = segSet.segmentNumberToIndex(segNo);
	    auto i_branch = segSet[segInd].branchNumber();
	    if (branch == i_branch) { 
		segNum = segNo;
	    }	
	}
	return segNum;
      }*/ 
    
    
     int firstConnectionInSegment(const Opm::CompletionSet& compSet, const Opm::SegmentSet& segSet, const size_t segIndex) {
	auto segNumber  = segSet[segIndex].segmentNumber();
	int firstConnection = std::numeric_limits<int>::max();
	//std::cout << "firstConnectionInSegment: segIndex" << segIndex  << " segNumber: " << segNumber << std::endl;
	for (auto it : compSet) {
	    auto c_Segment 	= it.getSegmentNumber();
	    auto c_SeqIndex 	= it.getSeqIndex();
	    auto c_i 		= it.getI();
	    auto c_j 		= it.getJ();
	    auto c_k 		= it.getK();
	    auto c_depth 	= it.getCenterDepth();
	    auto c_s_length 	= it.getCompSegStartLength();
	    //std::cout << "firstConnectionInSegment - i,j,k: " << c_i  << " , " << c_j << " , " << c_k << " c_depth " << c_depth << " c_s_length " << c_s_length << std::endl;
	    if ((segNumber == c_Segment) && (c_SeqIndex < firstConnection)) { 
		firstConnection = c_SeqIndex;
		//std::cout << "firstConnectionInSegment - firstConnection: " << firstConnection << std::endl;
	    }	
	}
	return (firstConnection == std::numeric_limits<int>::max()) ? 0: firstConnection+1;
    }
    
     int noConnectionsSegment(const Opm::CompletionSet& compSet, const Opm::SegmentSet& segSet, const size_t segIndex) {
	auto segNumber  = segSet[segIndex].segmentNumber();
	int noConnections = 0;
	//std::cout << "noConnectionsSegment: segIndex" << segIndex  << " segNumber: " << segNumber << std::endl;
	//for (auto it = compSet.begin(); it < compSet.end(); it++) {
	for (auto it : compSet) {
	    auto cSegment = it.getSegmentNumber();
	    if (segNumber == cSegment) { 
		noConnections+=1;
		//std::cout << "noConnectionsSegment - i,j,k: " << c_i  << " , " << c_j << " , " << c_k << std::endl;
	    }	
	}
	return noConnections;
    }
    
    std::vector<size_t> inflowSegmentsIndex(const Opm::SegmentSet& segSet, size_t segIndex) {
	auto segNumber  = segSet[segIndex].segmentNumber();
	std::vector<size_t> inFlowSegNum;
	//std::cout << "inflowSegmentsIndex for segIndex: " << segIndex  << " segNumber: " << segNumber << std::endl;
	for (size_t ind = 0; ind < segSet.numberSegment(); ind++) {
	    auto i_outletSeg = segSet[ind].outletSegment();
	    if (segNumber == i_outletSeg) { 
		inFlowSegNum.push_back(ind);
		//std::cout << "inflowSegmentsIndex ind: " << ind  << " i_outletSeg: " << i_outletSeg << std::endl;
	    }	
	}
	return inFlowSegNum;
    }

     int noInFlowBranches(const Opm::SegmentSet& segSet, size_t segIndex) {
	auto segNumber  = segSet[segIndex].segmentNumber();
	auto branch     = segSet[segIndex].branchNumber();
	int noIFBr = 0;
	//std::cout << "noOutFlowBranches for segIndex: " << segIndex  << std::endl;
	for (size_t ind = 0; ind < segSet.numberSegment(); ind++) {
	    auto o_segNum = segSet[ind].outletSegment();
	    auto i_branch = segSet[ind].branchNumber();
	    if ((segNumber == o_segNum) && (branch != i_branch)){ 
		noIFBr+=1;
		//std::cout << "Inflow branch: " << i_branch  << " Segment number " << o_segNum << std::endl;
	    }	
	}
	return noIFBr;
    }
    
    int sumNoInFlowBranches(const Opm::SegmentSet& segSet, size_t segIndex) {
	int sumIFB = 1;
	size_t indS = segIndex;
	bool nonSeg = false;
	//std::cout << "sumNoInFlowBranches - segIndex: " << segIndex  << std::endl;
	while ((indS >= 0) && (!nonSeg)) {
	    auto segNumber  = segSet[indS].segmentNumber();
	    auto branch     = segSet[indS].branchNumber();
	    auto outletS    = segSet[indS].outletSegment();
	    sumIFB += noInFlowBranches(segSet, indS);
	    //std::cout << "sumNoInFlowBranches - indS: " << indS  << " sumiFB: " << sumIFB << std::endl;
	    indS = segSet.segmentNumberToIndex(outletS);
	    nonSeg = (segNumber == 1) ? true : false;
	    //std::cout << "sumNoInFlowBranches - indS-new: " << indS  << std::endl;
	}
	return sumIFB;
    } 
    
    
    int noUpstreamSeg(const Opm::SegmentSet& segSet, size_t segIndex) {
	auto branch = segSet[segIndex].branchNumber();
	int noUpstrSeg  = 0;
	size_t ind = segIndex;
	size_t indMainBranch = segIndex;
	size_t oldIndMainBranch = segSet.numberSegment()+1;
	//std::cout << "noUpstreamSeg Init: segIndex: " << segIndex << " Branch: " << branch << std::endl;
	while (ind < segSet.numberSegment() && (indMainBranch != oldIndMainBranch)) {
	    oldIndMainBranch = indMainBranch;
	    auto inFlowSegsInd = inflowSegmentsIndex(segSet, ind);
	    if (inFlowSegsInd.size() > 0) {
		for (auto segI : inFlowSegsInd) {
		    auto u_segNum = segSet[segI].segmentNumber();
		    auto u_branch = segSet[segI].branchNumber(); 
		    if (branch == u_branch) {
			// upstream segment is on same branch - continue search on same branch
			noUpstrSeg +=1;
			//std::cout << "noUpstreamSeg Same branch, segI: " << segI << " Branch: " << u_branch << " noUpstrSeg: " << noUpstrSeg << std::endl;
			indMainBranch = segI;
		    } 
		    if (branch != u_branch)  {
			// upstream segment is on other branch - search along this branch to end of branch
			noUpstrSeg +=1;
			//std::cout << "noUpstreamSeg Other branch, segI: " << segI << " Branch: " << u_branch << " noUpstrSeg: " << noUpstrSeg << std::endl;
			int inc_noUpstrSeg = noUpstreamSeg(segSet, segI);
			noUpstrSeg += inc_noUpstrSeg;
			//std::cout << "noUpstreamSeg Other branch after recursive call, segI: " << segI << " Branch: " << u_branch << " noUpstrSeg: " << noUpstrSeg << std::endl;
		    }
		}
	    }
	    else {
		indMainBranch = segSet.numberSegment();
	    }
	ind = indMainBranch; 
	}
	//std::cout << "noUpstreamSeg Result: noUpstrSeg: " << noUpstrSeg << std::endl;
	return noUpstrSeg;
    }
    
    int inflowSegmentCurBranch(const Opm::SegmentSet& segSet, size_t segIndex) {
	auto branch = segSet[segIndex].branchNumber();
	auto segNumber  = segSet[segIndex].segmentNumber();
	int inFlowSegNum = 0;
	for (size_t ind = 0; ind < segSet.numberSegment(); ind++) {
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
        void staticContrib(const Opm::Well&            	well,
                           const std::size_t            rptStep,
			   const std::vector<int>& 	inteHead,
                           ISegArray&                   iSeg)
        {
            if (well.isMultiSegment(rptStep)) {
		std::cout << "AGG MSW, well name: " << well.name() << std::endl;
		//loop over segment set and print out information
		auto welSegSet = well.getSegmentSet(rptStep);
		auto completionSet = well.getCompletions(rptStep);
		auto noElmSeg = nisegz(inteHead);
		for (size_t ind_seg = 1; ind_seg <= welSegSet.numberSegment(); ind_seg++) {
		    auto ind = welSegSet.segmentNumberToIndex(ind_seg);
		    auto iS = (ind_seg-1)*noElmSeg;
		    iSeg[iS + 0] = noUpstreamSeg(welSegSet, ind);
		    iSeg[iS + 1] = welSegSet[ind].outletSegment();
		    iSeg[iS + 2] = inflowSegmentCurBranch(welSegSet, ind);
		    iSeg[iS + 3] = welSegSet[ind].branchNumber();
		    iSeg[iS + 4] = noInFlowBranches(welSegSet, ind);
		    iSeg[iS + 5] = sumNoInFlowBranches(welSegSet, ind);
		    iSeg[iS + 6] = noConnectionsSegment(completionSet, welSegSet, ind);
		    iSeg[iS + 7] = firstConnectionInSegment(completionSet, welSegSet, ind);
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
        void staticContrib(const Opm::Well& 		well,
                           const std::size_t    	rptStep,
			   const std::vector<int>& 	inteHead,
			   RSegArray& 			rSeg)
        {       
	    if (well.isMultiSegment(rptStep)) {
		std::cout << "AGG RSEG MSW, well name: " << well.name() << std::endl;
		//loop over segment set and print out information
		auto welSegSet = well.getSegmentSet(rptStep);
		auto completionSet = well.getCompletions(rptStep);
		auto noElmSeg = nrsegz(inteHead);
		std::cout << "AGG RSEG MSW, well : " << well.name() << std::endl;
		std::cout << "AGG RSEG MSW, well name: " << well.name() << std::endl;
		std::cout << "well name from segmentSet: " << welSegSet.wellName() << std::endl;
		std::cout << "number of items pr segment: "<< noElmSeg << std::endl;
		std::cout << "segment information pr. segment: "<< std::endl;
		    //treat the top segment individually
		    int ind_seg = 1;
		    auto ind = welSegSet.segmentNumberToIndex(ind_seg);
		    std::cout << "RSEG Segment no -ind_seg: " << ind_seg << ",  Segment index no - ind: " << ind << std::endl;
		    rSeg[0] = welSegSet.lengthTopSegment();
		    rSeg[1] = welSegSet.depthTopSegment();
		    rSeg[5] = welSegSet.volumeTopSegment();
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
		for (size_t ind_seg = 2; ind_seg <= welSegSet.numberSegment(); ind_seg++) {
		    // set the elements of the rSeg array
		    auto ind = welSegSet.segmentNumberToIndex(ind_seg);
		    auto outSeg = welSegSet[ind].outletSegment();
		    auto ind_ofs = welSegSet.segmentNumberToIndex(outSeg);
		    //std::cout << "RSEG outSeg: " << outSeg << ",  ind_ofs: " << ind_ofs << std::endl;
		    auto iS = (ind_seg-1)*noElmSeg;
		    std::cout << "RSEG Segment no -ind_seg: " << ind_seg << ",  Segment index no - ind: " << ind << std::endl;
		    rSeg[iS +   0] = welSegSet[ind].totalLength() - welSegSet[ind_ofs].totalLength();
		    rSeg[iS +   1] = welSegSet[ind].depth() - welSegSet[ind_ofs].depth();
		    rSeg[iS +   2] = welSegSet[ind].internalDiameter();
		    rSeg[iS +   3] = welSegSet[ind].roughness();
		    rSeg[iS +   4] = welSegSet[ind].crossArea();
		    rSeg[iS +   5] = welSegSet[ind].volume();
		    rSeg[iS +   6] = welSegSet[ind].totalLength();
		    rSeg[iS +   7] = welSegSet[ind].depth();
		    
		    		    //  segment pressure (to be added!!)
		    rSeg[iS +  39] = 0;
		    
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
        void staticContrib(const Opm::Well&                well,
                           const std::vector<std::string>& groupNames,
                           const int                       maxGroups,
                           const std::size_t               rptStep,
			   const std::vector<int>& 	   inteHead,
                           ILBSArray&                      iLBS)
        {
            iLBS[1 - 1] = 0;
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
        void staticContrib(const Opm::Well&                well,
                           const std::vector<std::string>& groupNames,
                           const int                       maxGroups,
                           const std::size_t               rptStep,
			   const std::vector<int>& 	   inteHead,
                           ILBRArray&                      iLBR)
        {
            iLBR[1 - 1] = 0;
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
captureDeclaredMSWData(const Schedule&   sched,
                        const std::size_t rptStep,
			const std::vector<int>& inteHead )
{
    const auto& wells = sched.getWells(rptStep);
    auto msw = std::vector<const Opm::Well*>{};
    //msw.reserve(wells.size());
    for (const auto well : wells) {
	if (well->isMultiSegment(rptStep)) msw.push_back(well);
    }

    // Extract Contributions to ISeg Array
     {
        MSWLoop(msw, [rptStep, inteHead, this]
            (const Well& well, const std::size_t mswID) -> void
        {
            auto imsw = this->iSeg_[mswID];

            ISeg::staticContrib(well, rptStep, inteHead, imsw);
        });
    }
    
        // Extract Contributions to RSeg Array
     {
        MSWLoop(msw, [rptStep, inteHead, this]
            (const Well& well, const std::size_t mswID) -> void
        {
            auto rmsw = this->rSeg_[mswID];

            RSeg::staticContrib(well, rptStep, inteHead, rmsw);
        });
    }
}

// ---------------------------------------------------------------------
