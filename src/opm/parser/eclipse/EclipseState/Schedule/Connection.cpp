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

#include <algorithm>
#include <cassert>
#include <vector>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>

namespace Opm {


    Connection::Connection(int i, int j , int k ,
                           int compnum,
                           double depth,
                           WellCompletion::StateEnum stateArg ,
                           double CF,
                           double Kh,
                           double rw,
                           const int satTableId,
                           const WellCompletion::DirectionEnum direction,
			   const std::size_t seqIndex)
        : direction(direction),
          center_depth(depth),
          open_state(stateArg),
          sat_tableId(satTableId),
          m_complnum( compnum ),
          m_CF(CF),
          m_Kh(Kh),
          m_rw(rw),
          ijk({i,j,k}),
          m_seqIndex(seqIndex)
    {}

    /*bool Connection::sameCoordinate(const Connection& other) const {
        if ((m_i == other.m_i) &&
            (m_j == other.m_j) &&
            (m_k == other.m_k))
            return true;
        else
            return false;
    }*/

    bool Connection::sameCoordinate(const int i, const int j, const int k) const {
        if ((ijk[0] == i) && (ijk[1] == j) && (ijk[2] == k)) {
            return true;
        } else {
            return false;
        }
    }



    int Connection::getI() const {
        return ijk[0];
    }

    int Connection::getJ() const {
        return ijk[1];
    }

    int Connection::getK() const {
        return ijk[2];
    }

    bool Connection::attachedToSegment() const {
        return (segment_number > 0);
    }
    
    const std::size_t& Connection::getSeqIndex() const {
        return m_seqIndex;
    }
    
    void Connection::setSeqIndex(std::size_t index) {
        m_seqIndex = index;
    }

    WellCompletion::DirectionEnum Connection::dir() const {
        return this->direction;
    }

    double Connection::depth() const {
        return this->center_depth;
    }

    WellCompletion::StateEnum Connection::state() const {
        return this->open_state;
    }

    int Connection::satTableId() const {
        return this->sat_tableId;
    }

    int Connection::complnum() const {
        return this->m_complnum;
    }

    void Connection::setComplnum(int complnum) {
        this->m_complnum = complnum;
    }

    double Connection::CF() const {
        return this->m_CF;
    }

    double Connection::Kh() const {
        return this->m_Kh;
    }

    double Connection::rw() const {
        return this->m_rw;
    }

    void Connection::setState(WellCompletion::StateEnum state) {
        this->open_state = state;
    }

  void Connection::updateSegment(int segment_number, double center_depth, std::size_t seqIndex) {
        this->segment_number = segment_number;
        this->center_depth = center_depth;
        this->seqIndex = seqIndex;
    }

    int Connection::segment() const {
        return this->segment_number;
    }

    void Connection::scaleWellPi(double wellPi) {
        this->wPi *= wellPi;
    }

    double Connection::wellPi() const {
        return this->wPi;
    }

    bool Connection::operator==( const Connection& rhs ) const {
        return this->ijk == rhs.ijk
            && this->m_complnum == rhs.m_complnum
            && this->m_CF == rhs.m_CF
            && this->m_rw == rhs.m_rw
            && this->wPi == rhs.wPi
            && this->m_Kh == rhs.m_Kh
            && this->sat_tableId == rhs.sat_tableId
            && this->open_state == rhs.open_state
            && this->direction == rhs.direction
            && this->segment_number == rhs.segment_number
            && this->center_depth == rhs.center_depth
            && this->m_seqIndex == rhs.m_seqIndex;
    }

    bool Connection::operator!=( const Connection& rhs ) const {
        return !( *this == rhs );
    }
}
