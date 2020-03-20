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
#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <opm/io/eclipse/rst/connection.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>


namespace Opm {


    Connection::Connection(int i, int j , int k ,
                           std::size_t global_index,
                           int compnum,
                           double depth,
                           State stateArg ,
                           double CF,
                           double Kh,
                           double rw,
                           double r0,
                           double skin_factor,
                           const int satTableId,
                           const Direction directionArg,
                           const CTFKind ctf_kind,
			   const std::size_t seqIndex,
			   const double segDistStart,
			   const double segDistEnd,
			   const bool defaultSatTabId)
        : direction(directionArg),
          center_depth(depth),
          open_state(stateArg),
          sat_tableId(satTableId),
          m_complnum( compnum ),
          m_CF(CF),
          m_Kh(Kh),
          m_rw(rw),
          m_r0(r0),
          m_skin_factor(skin_factor),
          ijk({i,j,k}),
          m_ctfkind(ctf_kind),
          m_global_index(global_index),
          m_seqIndex(seqIndex),
          m_segDistStart(segDistStart),
          m_segDistEnd(segDistEnd),
          m_defaultSatTabId(defaultSatTabId)
    {
    }

namespace {
constexpr bool defaultSatTabId = true;
constexpr int compseg_seqIndex = 1;
constexpr double def_wellPi = 1.0;
}

Connection::Connection(const RestartIO::RstConnection& rst_connection, std::size_t insert_index, const EclipseGrid& grid, const FieldPropsManager& fp) :
        direction(rst_connection.dir),
        center_depth(rst_connection.depth),
        open_state(rst_connection.state),
        sat_tableId(rst_connection.drain_sat_table),
        m_complnum(rst_connection.completion),
        m_CF(rst_connection.cf),
        m_Kh(rst_connection.kh),
        m_rw(rst_connection.diameter / 2),
        m_r0(rst_connection.r0),
        m_skin_factor(rst_connection.skin_factor),
        ijk(rst_connection.ijk),
        m_ctfkind(rst_connection.cf_kind),
        m_seqIndex(insert_index),
        m_segDistStart(rst_connection.segdist_start),
        m_segDistEnd(rst_connection.segdist_end),
        m_defaultSatTabId(defaultSatTabId),
        m_compSeg_seqIndex(compseg_seqIndex),
        segment_number(rst_connection.segment),
        wPi(def_wellPi)
    {
        if (this->m_defaultSatTabId) {
            const auto& satnum = fp.get_int("SATNUM");
            auto active_index = grid.activeIndex(this->ijk[0], this->ijk[1], this->ijk[2]);
            this->sat_tableId = satnum[active_index];
        }

    }


    Connection::Connection(Direction dir, double depth, State state,
                           int satTableId, int complnum, double CF,
                           double Kh, double rw, double r0, double skinFactor,
                           const std::array<int,3>& IJK,
                           std::size_t global_index,
                           CTFKind kind, std::size_t seqIndex,
                           double segDistStart, double segDistEnd,
                           bool defaultSatTabId, std::size_t compSegSeqIndex,
                           int segment, double wellPi)
        : direction(dir)
        , center_depth(depth)
        , open_state(state)
        , sat_tableId(satTableId)
        , m_complnum(complnum)
        , m_CF(CF)
        , m_Kh(Kh)
        , m_rw(rw)
        , m_r0(r0)
        , m_skin_factor(skinFactor)
        , ijk(IJK)
        , m_global_index(global_index)
        , m_ctfkind(kind)
        , m_seqIndex(seqIndex)
        , m_segDistStart(segDistStart)
        , m_segDistEnd(segDistEnd)
        , m_defaultSatTabId(defaultSatTabId)
        , m_compSeg_seqIndex(compSegSeqIndex)
        , segment_number(segment)
        , wPi(wellPi)
    {}

    Connection::Connection()
          : Connection(Direction::X, 1.0, State::SHUT,
                       0, 0, 0.0, 0.0, 0.0, 0.0, 0.0,
                      {0,0,0},0, CTFKind::Defaulted, 0, 0.0, 0.0, false, 0, 0, 0.0)
    {}

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

    std::size_t Connection::global_index() const {
        return this->m_global_index;
    }

    bool Connection::attachedToSegment() const {
        return (segment_number > 0);
    }

    const std::size_t& Connection::getSeqIndex() const {
        return m_seqIndex;
    }

    const bool& Connection::getDefaultSatTabId() const {
        return m_defaultSatTabId;
    }

    const std::size_t& Connection::getCompSegSeqIndex() const {
        return m_compSeg_seqIndex;
    }

    Connection::Direction Connection::dir() const {
        return this->direction;
    }

    const double& Connection::getSegDistStart() const {
        return m_segDistStart;
    }

    const double& Connection::getSegDistEnd() const {
        return m_segDistEnd;
    }


    void Connection::setDefaultSatTabId(bool id) {
        m_defaultSatTabId = id;
    }

    double Connection::depth() const {
        return this->center_depth;
    }

    Connection::State Connection::state() const {
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

    double Connection::r0() const {
        return this->m_r0;
    }

    double Connection::skinFactor() const {
        return this->m_skin_factor;
    }

    void Connection::setState(State state) {
        this->open_state = state;
    }

    void Connection::updateSegment(int segment_number_arg,
                                   double center_depth_arg,
                                   std::size_t compseg_insert_index,
                                   double start,
                                   double end) {
        this->segment_number = segment_number_arg;
        this->center_depth = center_depth_arg;
        this->m_compSeg_seqIndex = compseg_insert_index;
        this->m_segDistStart = start;
        this->m_segDistEnd = end;
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



    std::string Connection::str() const {
        std::stringstream ss;
        ss << "ijk: " << this->ijk[0] << ","  << this->ijk[1] << "," << this->ijk[2] << std::endl;
        ss << "COMPLNUM " << this->m_complnum << std::endl;
        ss << "CF " << this->m_CF << std::endl;
        ss << "RW " << this->m_rw << std::endl;
        ss << "R0 " << this->m_r0 << std::endl;
        ss << "skinf " << this->m_skin_factor << std::endl;
        ss << "wPi " << this->wPi << std::endl;
        ss << "kh " << this->m_Kh << std::endl;
        ss << "sat_tableId " << this->sat_tableId << std::endl;
        ss << "open_state " << Connection::State2String(this->open_state) << std::endl;
        ss << "direction " << Connection::Direction2String(this->direction) << std::endl;
        ss << "CTF Source " << Connection::CTFKindToString(this->m_ctfkind) << '\n';
        ss << "segment_nr " << this->segment_number << std::endl;
        ss << "center_depth " << this->center_depth << std::endl;
        ss << "seqIndex " << this->m_seqIndex << std::endl;

        return ss.str();
}

    bool Connection::operator==( const Connection& rhs ) const {
        return this->ijk == rhs.ijk
            && this->m_global_index == rhs.m_global_index
            && this->m_complnum == rhs.m_complnum
            && this->m_CF == rhs.m_CF
            && this->m_rw == rhs.m_rw
            && this->m_r0 == rhs.m_r0
            && this->m_skin_factor == rhs.m_skin_factor
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



const std::string Connection::State2String( State enumValue ) {
    switch( enumValue ) {
    case State::OPEN:
        return "OPEN";
    case State::AUTO:
        return "AUTO";
    case State::SHUT:
        return "SHUT";
    default:
        throw std::invalid_argument("Unhandled enum value");
    }
}


Connection::State Connection::StateFromString( const std::string& stringValue ) {
    if (stringValue == "OPEN")
        return State::OPEN;
    else if (stringValue == "SHUT")
        return State::SHUT;
    else if (stringValue == "STOP")
        return State::SHUT;
    else if (stringValue == "AUTO")
        return State::AUTO;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}


std::string Connection::Direction2String(const Direction enumValue)
{
    std::string stringValue;

    switch (enumValue) {
    case Direction::X:
        stringValue = "X";
        break;

    case Direction::Y:
        stringValue = "Y";
        break;

    case Direction::Z:
        stringValue = "Z";
        break;
    }

    return stringValue;
}


Connection::Direction Connection::DirectionFromString(const std::string& s )
{
    Direction direction;

    if      (s == "X") { direction = Direction::X; }
    else if (s == "Y") { direction = Direction::Y; }
    else if (s == "Z") { direction = Direction::Z; }
    else {
        std::string msg = "Unsupported completion direction " + s;
        throw std::invalid_argument(msg);
    }

    return direction;
}


const std::string Connection::Order2String( Order enumValue ) {
    switch( enumValue ) {
    case Order::DEPTH:
        return "DEPTH";
    case Order::INPUT:
        return "INPUT";
    case Order::TRACK:
        return "TRACK";
    default:
        throw std::invalid_argument("Unhandled enum value");
    }
}


Connection::Order Connection::OrderFromString(const std::string& stringValue ) {
    if (stringValue == "DEPTH")
        return Order::DEPTH;
    else if (stringValue == "INPUT")
        return Order::INPUT;
    else if (stringValue == "TRACK")
        return Order::TRACK;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}

std::string Connection::CTFKindToString(const CTFKind ctf_kind)
{
    switch (ctf_kind) {
        case CTFKind::DeckValue:
            return "DeckValue";

        case CTFKind::Defaulted:
            return "Defaulted";
    }

    throw std::invalid_argument {
        "Unhandled CTF Kind Value: " +
        std::to_string(static_cast<int>(ctf_kind))
    };
}

Connection::CTFKind Connection::kind() const {
    return m_ctfkind;
}

}
