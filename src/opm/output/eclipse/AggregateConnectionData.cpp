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

#include <opm/output/eclipse/AggregateConnectionData.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cassert>
#include <cstddef>
#include <iostream>
#include <cmath>

namespace VI = Opm::RestartIO::Helpers::VectorItems;

// #####################################################################
// Class Opm::RestartIO::Helpers::AggregateConnectionData
// ---------------------------------------------------------------------

namespace {
    std::size_t numWells(const std::vector<int>& inteHead)
    {
        return inteHead[VI::intehead::NWELLS];
    }

    std::size_t maxNumConn(const std::vector<int>& inteHead)
    {
        return inteHead[VI::intehead::NCWMAX];
    }
      
    std::map <std::size_t, const Opm::Connection*>  mapSeqIndexToConnection(const Opm::WellConnections& conns)
    {
	// make seqIndex to Connection map
	std::map <std::size_t, const Opm::Connection*> seqIndConnMap;
	//std::cout << "mapSeqIndexToConnection(" << std::endl;
	for (const auto & conn : conns) {
	    std::size_t sI = conn.getSeqIndex();
	    seqIndConnMap.insert(std::make_pair(sI, &conn));
	    //std::cout << "mCSSITC sI: " << sI  << " conn.getI,J,K" << conn.getI() << " , " << conn.getJ() << " , " << conn.getK() << std::endl;
	}
	return seqIndConnMap;
    }

    std::map <std::size_t, const Opm::Connection*>  mapCompSegSeqIndexToConnection(const Opm::WellConnections& conns)
    {
	// make CompSegSeqIndex to Connection map
	std::map <std::size_t, const Opm::Connection*> cs_seqIndConnMap;
	//std::cout << "mapCompSegSeqIndexToConnection" << std::endl;
	for (const auto & conn : conns) {
	    std::size_t sI = conn.getCompSegSeqIndex();
	    cs_seqIndConnMap.insert(std::make_pair(sI, &conn));
	    //std::cout << "mCSSITC sI: " << sI  << " conn.getI,J,K" << conn.getI() << " , " << conn.getJ() << " , " << conn.getK() << std::endl;
	}
	return cs_seqIndConnMap;
    }


    template <class ConnOp>
    void connectionLoop(const std::vector<const Opm::Well*>& wells,
                        const Opm::EclipseGrid&              grid,
                        const std::size_t                    sim_step,
                        ConnOp&&                             connOp)
    {
        for (auto nWell = wells.size(), wellID = 0*nWell;
             wellID < nWell; ++wellID)
        {
            const auto* well = wells[wellID];

            if (well == nullptr) { continue; }
            
            const auto& conns = well->getActiveConnections(sim_step, grid);
	    std::map <std::size_t, const Opm::Connection*> sIToConn;

	    //Branch according to MSW well or not and 
	    //sort active connections according to appropriate seqIndex
	    if (well->isMultiSegment(sim_step)) {
		//sort connections according to input sequence in COMPSEGS
		sIToConn = mapCompSegSeqIndexToConnection(conns);
	    } else 
	    {
		//sort connections according to input sequence in COMPDAT
		sIToConn = mapSeqIndexToConnection(conns);
	    }
	    std::vector<const Opm::Connection*> connSI;
	    int niSI = well->getConnections(sim_step).size();
	    //std::cout << "ConnOP - well" << well->name() << " niSI: " << niSI << std::endl;
	    for (int iSI = 0; iSI < niSI; iSI++) {
		const auto searchSI = sIToConn.find(static_cast<std::size_t>(iSI));
		if (searchSI != sIToConn.end()) {		  
		  connSI.push_back(searchSI->second);
		  //std::cout << "ConnOP - iSI " << iSI  << " searchSI->second.getI,J,K" << searchSI->second->getI() << " , " << searchSI->second->getJ() << " , " << searchSI->second->getK() << std::endl;
		}
	    }
	    for (auto nConn = connSI.size(), connID = 0*nConn;
                 connID < nConn; ++connID)
            {
                connOp(*well, wellID, *(connSI[connID]), connID);
            }
        }
    }

    namespace IConn {
        std::size_t entriesPerConn(const std::vector<int>& inteHead)
        {
            return inteHead[VI::intehead::NICONZ];
        }

        Opm::RestartIO::Helpers::WindowedMatrix<int>
        allocate(const std::vector<int>& inteHead)
        {
            using WM = Opm::RestartIO::Helpers::WindowedMatrix<int>;

            return WM {
                WM::NumRows   { numWells(inteHead) },
                WM::NumCols   { maxNumConn(inteHead) },
                WM::WindowSize{ entriesPerConn(inteHead) }
            };
        }

        template <class IConnArray>
        void staticContrib(const Opm::Connection& conn,
                           const std::size_t      connID,
                           IConnArray&            iConn)
        {
            using ConnState = ::Opm::WellCompletion::StateEnum;

            // Wrong.  Should be connection's order of appearance in COMPDAT.
            iConn[0] = conn.getSeqIndex()+1;

            iConn[1] = conn.getI() + 1;
            iConn[2] = conn.getJ() + 1;
            iConn[3] = conn.getK() + 1;
            iConn[5] = (conn.state == ConnState::OPEN)
                ? 1 : -1000;

            iConn[6] = conn.getDefaultSatTabId() ? 0 : conn.sat_tableId;

            // Don't support differing sat-func tables for
            // draining and imbibition curves at connections.
            iConn[9] = iConn[6];

            iConn[12] = std::abs(conn.complnum);
            iConn[13] = conn.dir;
            iConn[14] = conn.attachedToSegment()
                ? conn.segment_number : 0;
        }
    } // IConn

    namespace SConn {
        std::size_t entriesPerConn(const std::vector<int>& inteHead)
        {
            return inteHead[VI::intehead::NSCONZ];
        }

        Opm::RestartIO::Helpers::WindowedMatrix<float>
        allocate(const std::vector<int>& inteHead)
        {
            using WM = Opm::RestartIO::Helpers::WindowedMatrix<float>;

            return WM {
                WM::NumRows   { numWells(inteHead) },
                WM::NumCols   { maxNumConn(inteHead) },
                WM::WindowSize{ entriesPerConn(inteHead) }
            };
        }

        template <class SConnArray>
        void staticContrib(const Opm::Connection& conn,
                           const Opm::UnitSystem& units,
                           SConnArray&            sConn)
        {
            using M = ::Opm::UnitSystem::measure;

            auto scprop = [&units](const M u, const double x) -> float
            {
                return static_cast<float>(units.from_si(u, x));
            };

            {
                const auto& ctf = conn
                    .getConnectionTransmissibilityFactorAsValueObject();

                if (ctf.hasValue()) {
                    sConn[0] = scprop(M::transmissibility, ctf.getValue());
                }
            }

            sConn[1] = scprop(M::length, conn.center_depth);
            sConn[2] = scprop(M::length, conn.getDiameter());

            {
                const auto& ckh = conn
                    .getEffectiveKhAsValueObject();

                if (ckh.hasValue()) {
                    auto tkh = scprop(M::permeability, ckh.getValue());
		    sConn[3] = scprop(M::length, tkh);
                }
            }
	    
	    

            sConn[11] = sConn[0];

            // sConn[20] and sConn[21] are tubing end/start (yes, 20 is
            // end, 21 is start) lengths of the current connection in a
            // multisegmented well.  That information is impossible to
            // reconstruct here since it is discared in member function
            // ::Opm::Well::handleCOMPSEGS().
	    sConn[20] = static_cast<float>(conn.getSegDistEnd());
	    sConn[21] = static_cast<float>(conn.getSegDistStart());

            sConn[29] = -1.0e+20f;
            sConn[30] = -1.0e+20f;
        }
    } // SConn
} // Anonymous

Opm::RestartIO::Helpers::AggregateConnectionData::
AggregateConnectionData(const std::vector<int>& inteHead)
    : iConn_(IConn::allocate(inteHead))
    , sConn_(SConn::allocate(inteHead))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateConnectionData::
captureDeclaredConnData(const Schedule&    sched,
                        const EclipseGrid& grid,
                        const UnitSystem&  units,
                        const std::size_t  sim_step)
{
    //const auto& actnum = grid.activeIndex;
    const auto& wells = sched.getWells(sim_step);

    connectionLoop(wells, grid, sim_step, [&units, this]
        (const Well&    /* well */, const std::size_t wellID,
         const Connection& conn   , const std::size_t connID) -> void
    {
        auto ic = this->iConn_(wellID, connID);
        auto sc = this->sConn_(wellID, connID);
        IConn::staticContrib(conn, connID, ic);
        SConn::staticContrib(conn, units, sc);
    });
}
