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


#ifndef COMPLETION_HPP_
#define COMPLETION_HPP_

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>

namespace Opm {

namespace RestartIO {
    struct RstConnection;
}

    class DeckKeyword;
    class DeckRecord;
    class EclipseGrid;
    class FieldPropsManager;

    class Connection {
    public:
        enum class State {
            OPEN = 1,
            SHUT = 2,
            AUTO = 3   // Seems like the AUTO state can not be serialized to restart files.
        };

        static const std::string State2String( State enumValue );
        static State StateFromString( const std::string& stringValue );


        enum class Direction{
            X = 1,
            Y = 2,
            Z = 3
        };

        static std::string Direction2String(const Direction enumValue);
        static Direction   DirectionFromString(const std::string& stringValue);


        enum class Order {
                          DEPTH,
                          INPUT,
                          TRACK
        };

        static const std::string Order2String( Order enumValue );
        static Order OrderFromString(const std::string& comporderStringValue);

        enum class CTFKind {
            DeckValue,
            Defaulted,
        };


        Connection();
        Connection(int i, int j , int k ,
                   std::size_t global_index,
                   int complnum,
                   double depth,
                   State state,
                   double CF,
                   double Kh,
                   double rw,
                   double r0,
                   double skin_factor,
                   const int satTableId,
                   const Direction direction,
                   const CTFKind ctf_kind,
                   const std::size_t seqIndex,
                   const double segDistStart,
                   const double segDistEnd,
                   const bool defaultSatTabId);

        Connection(Direction dir,
                   double depth,
                   State state,
                   int satTableId,
                   int complnum,
                   double CF,
                   double Kh,
                   double rw,
                   double r0,
                   double skinFactor,
                   const std::array<int,3>& IJK,
                   std::size_t global_index,
                   CTFKind kind,
                   std::size_t seqIndex,
                   double segDistStart,
                   double segDistEnd,
                   bool defaultSatTabId,
                   std::size_t compSegSeqIndex,
                   int segment,
                   double wellPi);

        Connection(const RestartIO::RstConnection& rst_connection, std::size_t insert_index, const EclipseGrid& grid, const FieldPropsManager& fp);

        bool attachedToSegment() const;
        bool sameCoordinate(const int i, const int j, const int k) const;
        int getI() const;
        int getJ() const;
        int getK() const;
        std::size_t global_index() const;
        State state() const;
        Direction dir() const;
        double depth() const;
        int satTableId() const;
        int complnum() const;
        int segment() const;
        double CF() const;
        double Kh() const;
        double rw() const;
        double r0() const;
        double skinFactor() const;
        double wellPi() const;
        CTFKind kind() const;

        void setState(State state);
        void setComplnum(int compnum);
        void scaleWellPi(double wellPi);
        void updateSegment(int segment_number_arg,
                           double center_depth_arg,
                           std::size_t compseg_insert_index,
                           double start,
                           double end);
        const std::size_t& getSeqIndex() const;
        const bool& getDefaultSatTabId() const;
        const std::size_t& getCompSegSeqIndex() const;
        void setDefaultSatTabId(bool id);
        const double& getSegDistStart() const;
        const double& getSegDistEnd() const;
        std::string str() const;
        bool ctfAssignedFromInput() const
        {
            return this->m_ctfkind == CTFKind::DeckValue;
        }

        bool operator==( const Connection& ) const;
        bool operator!=( const Connection& ) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(direction);
            serializer(center_depth);
            serializer(open_state);
            serializer(sat_tableId);
            serializer(m_complnum);
            serializer(m_CF);
            serializer(m_Kh);
            serializer(m_rw);
            serializer(m_r0);
            serializer(m_skin_factor);
            serializer(ijk);
            serializer(m_global_index);
            serializer(m_ctfkind);
            serializer(m_seqIndex);
            serializer(m_segDistStart);
            serializer(m_segDistEnd);
            serializer(m_defaultSatTabId);
            serializer(m_compSeg_seqIndex);
            serializer(segment_number);
            serializer(wPi);
        }

    private:
        Direction direction;
        double center_depth;
        State open_state;
        int sat_tableId;
        int m_complnum;
        double m_CF;
        double m_Kh;
        double m_rw;
        double m_r0;
        double m_skin_factor;

        std::array<int,3> ijk;
        CTFKind m_ctfkind;
        std::size_t m_global_index;
        std::size_t m_seqIndex;
        double m_segDistStart;
        double m_segDistEnd;
        bool m_defaultSatTabId;
        std::size_t m_compSeg_seqIndex=0;

        // related segment number
        // 0 means the completion is not related to segment
        int segment_number = 0;
        double wPi = 1.0;

        static std::string CTFKindToString(const CTFKind);
    };
}

#endif /* COMPLETION_HPP_ */

