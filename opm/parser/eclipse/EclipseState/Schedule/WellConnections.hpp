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


#ifndef CONNECTIONSET_HPP_
#define CONNECTIONSET_HPP_

#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>

namespace Opm {
    class EclipseGrid;
    class Eclipse3DProperties;
    class WellConnections {
    public:
        WellConnections() = default;
        WellConnections(int headI, int headJ);
        // cppcheck-suppress noExplicitConstructor
        WellConnections(const WellConnections& src, const EclipseGrid& grid);
        void addConnection(int i, int j , int k ,
                           double depth,
                           WellCompletion::StateEnum state ,
                           double CF,
                           double Kh,
                           double rw,
                           const int satTableId,
                           const WellCompletion::DirectionEnum direction = WellCompletion::DirectionEnum::Z,
			   const std::size_t seqIndex = 0);
        void loadCOMPDAT(const DeckRecord& record, const EclipseGrid& grid, const Eclipse3DProperties& eclipseProperties);

        using const_iterator = std::vector< Connection >::const_iterator;

        void add( Connection );
        size_t size() const;
        const Connection& operator[](size_t index) const;
        const Connection& get(size_t index) const;
        const Connection& getFromIJK(const int i, const int j, const int k) const;
        Connection& getFromIJK(const int i, const int j, const int k);

        const_iterator begin() const { return this->m_connections.begin(); }
        const_iterator end() const { return this->m_connections.end(); }
        void filter(const EclipseGrid& grid);
        bool allConnectionsShut() const;
        /// Order connections irrespective of input order.
        /// The algorithm used is the following:
        ///     1. The connection nearest to the given (well_i, well_j)
        ///        coordinates in terms of the connection's (i, j) is chosen
        ///        to be the first connection. If non-unique, choose one with
        ///        lowest z-depth (shallowest).
        ///     2. Choose next connection to be nearest to current in (i, j) sense.
        ///        If non-unique choose closest in z-depth (not logical cartesian k).
        ///
        /// \param[in] well_i  logical cartesian i-coordinate of well head
        /// \param[in] well_j  logical cartesian j-coordinate of well head
        /// \param[in] grid    EclipseGrid object, used for cell depths
        void orderConnections(size_t well_i, size_t well_j);

        bool operator==( const WellConnections& ) const;
        bool operator!=( const WellConnections& ) const;

    private:
        void addConnection(int i, int j , int k ,
                           int complnum,
                           double depth,
                           WellCompletion::StateEnum state ,
                           double CF,
                           double Kh,
                           double rw,
                           const int satTableId,
                           const WellCompletion::DirectionEnum direction = WellCompletion::DirectionEnum::Z,
			   const std::size_t seqIndex=0);

        std::vector< Connection > m_connections;
        size_t findClosestConnection(int oi, int oj, double oz, size_t start_pos);
        int headI, headJ;
    };
}



#endif
