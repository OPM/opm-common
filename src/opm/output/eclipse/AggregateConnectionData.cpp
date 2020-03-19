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

#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>

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

    template <class ConnOp>
    void connectionLoop(const std::vector<Opm::Well>& wells,
                        const Opm::EclipseGrid&        grid,
                        ConnOp&&                       connOp)
    {
        for (auto nWell = wells.size(), wellID = 0*nWell;
             wellID < nWell; ++wellID)
        {
            const auto& well = wells[wellID];
            std::vector<const Opm::Connection*> connSI;
            for (const auto& conn : well.getConnections()) {
                if (grid.cellActive(conn.getI(), conn.getJ(), conn.getK()))
                    connSI.push_back( &conn );
            }

            //Branch according to MSW well or not and
            //sort active connections according to appropriate seqIndex
            if (well.isMultiSegment()) {
                //sort connections according to input sequence in COMPSEGS
                std::sort(connSI.begin(), connSI.end(), [](const Opm::Connection* conn1, const Opm::Connection* conn2)
                                                            {
                                                                return conn1->getCompSegSeqIndex() < conn2->getCompSegSeqIndex();
                                                            });
            } else {
                std::sort(connSI.begin(), connSI.end(), [](const Opm::Connection* conn1, const Opm::Connection* conn2)
                                                        {
                                                            return conn1->getSeqIndex() < conn2->getSeqIndex();
                                                        });
            }


            for (auto nConn = connSI.size(), connID = 0*nConn;
                 connID < nConn; ++connID)
            {
                connOp(well, wellID, *(connSI[connID]), connID);
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
            using ConnState = ::Opm::Connection::State;
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::IConn::index;

            iConn[Ix::SeqIndex] = connID + 1;

            iConn[Ix::CellI] = conn.getI() + 1;
            iConn[Ix::CellJ] = conn.getJ() + 1;
            iConn[Ix::CellK] = conn.getK() + 1;

            iConn[Ix::ConnStat] = (conn.state() == ConnState::OPEN)
                ? 1 : 0;

            iConn[Ix::Drainage] = conn.getDefaultSatTabId()
                ? 0 : conn.satTableId();

            // Don't support differing sat-func tables for
            // draining and imbibition curves at connections.
            iConn[Ix::Imbibition] = iConn[Ix::Drainage];

            //complnum is(1 too large): 1 - based while icon is 0 - based?
            iConn[Ix::ComplNum] = conn.complnum();
            //iConn[Ix::ComplNum] = iConn[Ix::SeqIndex];

            iConn[Ix::ConnDir] = static_cast<int>(conn.dir());
            iConn[Ix::Segment] = conn.attachedToSegment()
                ? conn.segment() : 0;
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
            using M  = ::Opm::UnitSystem::measure;
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::SConn::index;

            auto scprop = [&units](const M u, const double x) -> float
            {
                return static_cast<float>(units.from_si(u, x));
            };

            sConn[Ix::ConnTrans] =
                scprop(M::transmissibility, conn.CF());

            sConn[Ix::Depth]    = scprop(M::length, conn.depth());
            sConn[Ix::Diameter] = scprop(M::length, 2*conn.rw());

            sConn[Ix::EffectiveKH] =
                scprop(M::effective_Kh, conn.Kh());

            sConn[Ix::SkinFactor] = conn.skinFactor(); 

            sConn[Ix::item12] = sConn[Ix::ConnTrans];

            sConn[Ix::SegDistEnd]   = scprop(M::length, conn.getSegDistEnd());
            sConn[Ix::SegDistStart] = scprop(M::length, conn.getSegDistStart());

            sConn[Ix::item30] = -1.0e+20f;
            sConn[Ix::item31] = -1.0e+20f;
            sConn[Ix::CFInDeck] = (conn.ctfAssignedFromInput()) ? 1 : 0;
        }
    } // SConn

    namespace XConn {
        std::size_t entriesPerConn(const std::vector<int>& inteHead)
        {
            return inteHead[VI::intehead::NXCONZ];
        }

        Opm::RestartIO::Helpers::WindowedMatrix<double>
        allocate(const std::vector<int>& inteHead)
        {
            using WM = Opm::RestartIO::Helpers::WindowedMatrix<double>;

            return WM {
                WM::NumRows   { numWells(inteHead) },
                WM::NumCols   { maxNumConn(inteHead) },
                WM::WindowSize{ entriesPerConn(inteHead) }
            };
        }

        template <class XConnArray>
        void dynamicContrib(const Opm::data::Connection& x,
                            const Opm::UnitSystem&       units,
                            XConnArray&                  xConn)
        {
            using M  = ::Opm::UnitSystem::measure;
            using Ix = ::Opm::RestartIO::Helpers::VectorItems::XConn::index;
            using R  = ::Opm::data::Rates::opt;

            xConn[Ix::Pressure] = units.from_si(M::pressure, x.pressure);

            // Note flow rate sign.  Treat production rates as positive.
            const auto& Q = x.rates;

            if (Q.has(R::oil)) {
                xConn[Ix::OilRate] =
                    - units.from_si(M::liquid_surface_rate, Q.get(R::oil));
            }

            if (Q.has(R::wat)) {
                xConn[Ix::WaterRate] =
                    - units.from_si(M::liquid_surface_rate, Q.get(R::wat));
            }

            if (Q.has(R::gas)) {
                xConn[Ix::GasRate] =
                    - units.from_si(M::gas_surface_rate, Q.get(R::gas));
            }

            xConn[Ix::ResVRate] = 0.0;

            if (Q.has(R::reservoir_oil)) {
                xConn[Ix::ResVRate] -=
                    units.from_si(M::rate, Q.get(R::reservoir_oil));
            }

            if (Q.has(R::reservoir_water)) {
                xConn[Ix::ResVRate] -=
                    units.from_si(M::rate, Q.get(R::reservoir_water));
            }

            if (Q.has(R::reservoir_gas)) {
                xConn[Ix::ResVRate] -=
                    units.from_si(M::rate, Q.get(R::reservoir_gas));
            }
        }
    } // XConn
} // Anonymous

Opm::RestartIO::Helpers::AggregateConnectionData::
AggregateConnectionData(const std::vector<int>& inteHead)
    : iConn_(IConn::allocate(inteHead))
    , sConn_(SConn::allocate(inteHead))
    , xConn_(XConn::allocate(inteHead))
{}

// ---------------------------------------------------------------------

void
Opm::RestartIO::Helpers::AggregateConnectionData::
captureDeclaredConnData(const Schedule&        sched,
                        const EclipseGrid&     grid,
                        const UnitSystem&      units,
                        const data::WellRates& xw,
                        const std::size_t      sim_step)
{
    using ConnectionRates = std::vector<const Opm::data::Connection*>;
    const auto& wells = sched.getWells(sim_step);
    //
    // construct a composite vector of connection objects  holding
    // rates for all open connectons
    //
    std::map<std::string, ConnectionRates> allRates;
    for (const auto& wl : wells) {
        const auto conns = WellConnections(wl.getConnections(), grid);

        allRates[wl.name()] = ConnectionRates{conns.size(), nullptr};
        const auto rates_iter = xw.find(wl.name());
        size_t rCInd = 0;
        if (rates_iter != xw.end()) {
            auto& well_rates = allRates[wl.name()];
            for (auto nConn = conns.size(), connID = 0*nConn; connID < nConn; connID++) {
                const auto& conn = conns[connID];

                // WellRates connections are only defined for OPEN connections
                if (conn.state() != Opm::Connection::State::OPEN)
                    continue;

                const auto& connection_rates = rates_iter->second.connections;

                if (rCInd < connection_rates.size()) {
                    well_rates[connID] = &(connection_rates[rCInd]);
                    rCInd += 1;
                } else
                    throw std::invalid_argument {
                        "Inconsistent number of open connections I in vector<Opm::data::Connection*> (" +
                            std::to_string(connection_rates.size()) + ") in Well " + wl.name()
                    };
            }
        }
    }

    connectionLoop(wells, grid, [&units, &allRates, this]
        (const Well&      well, const std::size_t wellID,
         const Connection& conn, const std::size_t connID) -> void
    {
        auto ic = this->iConn_(wellID, connID);
        auto sc = this->sConn_(wellID, connID);

        IConn::staticContrib(conn, connID, ic);
        SConn::staticContrib(conn, units, sc);

        auto xi = allRates.find(well.name());
        if ((xi != allRates.end()) &&
            (connID < xi->second.size()))
            //(connID < xi->second.connections.size()))
        {
            auto xc = this->xConn_(wellID, connID);

            //XConn::dynamicContrib(xi->second.connections[connID],
            if (xi->second[connID]) XConn::dynamicContrib(*(xi->second[connID]), units, xc);
        }
    });
}
