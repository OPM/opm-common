/*
  Copyright 2013 Statoil ASA.

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

#include <cassert>
#include <cmath>
#include <limits>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellConnections.hpp>

namespace Opm {

    WellConnections::WellConnections(int headIArg, int headJArg) :
        headI(headIArg),
        headJ(headJArg)
    {
    }



    WellConnections::WellConnections(const WellConnections& src, const EclipseGrid& grid) {
        for (const auto& c : src) {
            if (grid.cellActive(c.getI(), c.getJ(), c.getK()))
                this->add(c);
        }
    }

    void WellConnections::addConnection(int i, int j , int k ,
                                        int complnum,
                                        double depth,
                                        WellCompletion::StateEnum state ,
                                        const Value<double>& connectionTransmissibilityFactor,
                                        const Value<double>& diameter,
                                        const Value<double>& skinFactor,
                                        const Value<double>& Kh,
                                        const int satTableId,
                                        const WellCompletion::DirectionEnum direction)
    {
        int conn_i = (i < 0) ? this->headI : i;
        int conn_j = (j < 0) ? this->headJ : j;
        Connection conn(conn_i, conn_j, k, complnum, depth, state, connectionTransmissibilityFactor, diameter, skinFactor, Kh, satTableId, direction);
        this->add(conn);
    }



    void WellConnections::addConnection(int i, int j , int k ,
                                        double depth,
                                        WellCompletion::StateEnum state ,
                                        const Value<double>& connectionTransmissibilityFactor,
                                        const Value<double>& diameter,
                                        const Value<double>& skinFactor,
                                        const Value<double>& Kh,
                                        const int satTableId,
                                        const WellCompletion::DirectionEnum direction)
    {
        int complnum = -(this->m_connections.size() + 1);
        this->addConnection(i,
                            j,
                            k,
                            complnum,
                            depth,
                            state,
                            connectionTransmissibilityFactor,
                            diameter,
                            skinFactor,
                            Kh,
                            satTableId,
                            direction);
    }

    void WellConnections::loadCOMPDAT(const DeckRecord& record, const EclipseGrid& grid, const Eclipse3DProperties& eclipseProperties) {
        const auto& itemI = record.getItem( "I" );
        const auto defaulted_I = itemI.defaultApplied( 0 ) || itemI.get< int >( 0 ) == 0;
        const int I = !defaulted_I ? itemI.get< int >( 0 ) - 1 : this->headI;

        const auto& itemJ = record.getItem( "J" );
        const auto defaulted_J = itemJ.defaultApplied( 0 ) || itemJ.get< int >( 0 ) == 0;
        const int J = !defaulted_J ? itemJ.get< int >( 0 ) - 1 : this->headJ;

        int K1 = record.getItem("K1").get< int >(0) - 1;
        int K2 = record.getItem("K2").get< int >(0) - 1;
        WellCompletion::StateEnum state = WellCompletion::StateEnumFromString( record.getItem("STATE").getTrimmedString(0) );
        Value<double> connectionTransmissibilityFactor("CompletionTransmissibilityFactor");
        Value<double> diameter("Diameter");
        Value<double> skinFactor("SkinFactor");
        Value<double> Kh("Kh");
        const auto& satnum = eclipseProperties.getIntGridProperty("SATNUM");
        int satTableId = -1;
        bool defaultSatTable = true;
        {
            const auto& connectionTransmissibilityFactorItem = record.getItem("CONNECTION_TRANSMISSIBILITY_FACTOR");
            const auto& diameterItem = record.getItem("DIAMETER");
            const auto& skinFactorItem = record.getItem("SKIN");
            const auto& KhItem = record.getItem("Kh");
            const auto& satTableIdItem = record.getItem("SAT_TABLE");

            if (connectionTransmissibilityFactorItem.hasValue(0) && connectionTransmissibilityFactorItem.getSIDouble(0) > 0)
                connectionTransmissibilityFactor.setValue(connectionTransmissibilityFactorItem.getSIDouble(0));

            if (diameterItem.hasValue(0))
                diameter.setValue( diameterItem.getSIDouble(0));

            if (skinFactorItem.hasValue(0))
                skinFactor.setValue( skinFactorItem.get< double >(0));

            if (KhItem.hasValue(0) && (KhItem.get< double >(0) > 0.0))
                Kh.setValue( KhItem.getSIDouble(0));

            if (satTableIdItem.hasValue(0) && satTableIdItem.get < int > (0) > 0)
            {
                satTableId = satTableIdItem.get< int >(0);
                defaultSatTable = false;
            }
        }

        const WellCompletion::DirectionEnum direction = WellCompletion::DirectionEnumFromString(record.getItem("DIR").getTrimmedString(0));

        for (int k = K1; k <= K2; k++) {
            if (defaultSatTable)
                satTableId = satnum.iget(grid.getGlobalIndex(I,J,k));

            auto same_ijk = [&]( const Connection& c ) {
                return c.sameCoordinate( I,J,k );
            };

            auto prev = std::find_if( this->m_connections.begin(),
                                      this->m_connections.end(),
                                      same_ijk );

            if (prev == this->m_connections.end()) {
                this->addConnection(I,J,k,
                                    grid.getCellDepth( I,J,k ),
                                    state,
                                    connectionTransmissibilityFactor,
                                    diameter,
                                    skinFactor,
                                    Kh,
                                    satTableId,
                                    direction );
            } else {
                // The complnum value carries over; the rest of the state is fully specified by
                // the current COMPDAT keyword.
                int complnum = prev->complnum;
                *prev = Connection(I,J,k,
                                   complnum,
                                   grid.getCellDepth(I,J,k),
                                   state,
                                   connectionTransmissibilityFactor,
                                   diameter,
                                   skinFactor,
                                   Kh,
                                   satTableId,
                                   direction );
            }
        }
    }




    size_t WellConnections::size() const {
        return m_connections.size();
    }

    const Connection& WellConnections::get(size_t index) const {
        return this->m_connections.at( index );
    }

    const Connection& WellConnections::getFromIJK(const int i, const int j, const int k) const {
        for (size_t ic = 0; ic < size(); ++ic) {
            if (get(ic).sameCoordinate(i, j, k)) {
                return get(ic);
            }
        }
        throw std::runtime_error(" the connection is not found! \n ");
    }


    Connection& WellConnections::getFromIJK(const int i, const int j, const int k) {
      for (size_t ic = 0; ic < size(); ++ic) {
        if (get(ic).sameCoordinate(i, j, k)) {
          return this->m_connections[ic];
        }
      }
      throw std::runtime_error(" the connection is not found! \n ");
    }


    void WellConnections::add( Connection connection ) {
        m_connections.emplace_back( connection );
    }

    bool WellConnections::allConnectionsShut( ) const {
        auto shut = []( const Connection& c ) {
            return c.state == WellCompletion::StateEnum::SHUT;
        };

        return std::all_of( this->m_connections.begin(),
                            this->m_connections.end(),
                            shut );
    }



    void WellConnections::orderConnections(size_t well_i, size_t well_j)
    {
        if (m_connections.empty()) {
            return;
        }

        // Find the first connection and swap it into the 0-position.
        const double surface_z = 0.0;
        size_t first_index = findClosestConnection(well_i, well_j, surface_z, 0);
        std::swap(m_connections[first_index], m_connections[0]);

        // Repeat for remaining connections.
        //
        // Note that since findClosestConnection() is O(n), this is an
        // O(n^2) algorithm. However, it should be acceptable since
        // the expected number of connections is fairly low (< 100).

        if( this->m_connections.empty() ) return;

        for (size_t pos = 1; pos < m_connections.size() - 1; ++pos) {
            const auto& prev = m_connections[pos - 1];
            const double prevz = prev.center_depth;
            size_t next_index = findClosestConnection(prev.getI(), prev.getJ(), prevz, pos);
            std::swap(m_connections[next_index], m_connections[pos]);
        }
    }



    size_t WellConnections::findClosestConnection(int oi, int oj, double oz, size_t start_pos)
    {
        size_t closest = std::numeric_limits<size_t>::max();
        int min_ijdist2 = std::numeric_limits<int>::max();
        double min_zdiff = std::numeric_limits<double>::max();
        for (size_t pos = start_pos; pos < m_connections.size(); ++pos) {
            const auto& connection = m_connections[ pos ];

            const double depth = connection.center_depth;
            const int ci = connection.getI();
            const int cj = connection.getJ();
            // Using square of distance to avoid non-integer arithmetics.
            const int ijdist2 = (ci - oi) * (ci - oi) + (cj - oj) * (cj - oj);
            if (ijdist2 < min_ijdist2) {
                min_ijdist2 = ijdist2;
                min_zdiff = std::abs(depth - oz);
                closest = pos;
            } else if (ijdist2 == min_ijdist2) {
                const double zdiff = std::abs(depth - oz);
                if (zdiff < min_zdiff) {
                    min_zdiff = zdiff;
                    closest = pos;
                }
            }
        }
        assert(closest != std::numeric_limits<size_t>::max());
        return closest;
    }

    bool WellConnections::operator==( const WellConnections& rhs ) const {
        return this->size() == rhs.size()
            && std::equal( this->begin(), this->end(), rhs.begin() );
    }

    bool WellConnections::operator!=( const WellConnections& rhs ) const {
        return !( *this == rhs );
    }


    void WellConnections::filter(const EclipseGrid& grid) {
        auto new_end = std::remove_if(m_connections.begin(),
                                      m_connections.end(),
                                      [&grid](const Connection& c) { return !grid.cellActive(c.getI(), c.getJ(), c.getK()); });
        m_connections.erase(new_end, m_connections.end());
    }
}
