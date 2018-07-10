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
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>

namespace Opm {

    class DeckKeyword;
    class DeckRecord;

    class Connection {
    public:
        Connection(int i, int j , int k ,
                   int complnum,
                   double depth,
                   WellCompletion::StateEnum state,
                   double CF,
                   double Kh,
                   double rw,
                   const int satTableId,
                   const WellCompletion::DirectionEnum direction,
		   const std::size_t seqIndex);


        bool attachedToSegment() const;
        bool sameCoordinate(const int i, const int j, const int k) const;
        int getI() const;
        int getJ() const;
        int getK() const;
        WellCompletion::StateEnum state() const;
        WellCompletion::DirectionEnum dir() const;
        double depth() const;
        int satTableId() const;
        int complnum() const;
        int segment() const;
        double CF() const;
        double Kh() const;
        double rw() const;
        double wellPi() const;

        void setState(WellCompletion::StateEnum state);
        void setComplnum(int compnum);
        void scaleWellPi(double wellPi);
        void updateSegment(int segment_number, double center_depth, std::size_t seqIndex);
	const std::size_t& getSeqIndex() const;
	void setSeqIndex(std::size_t index);

        bool operator==( const Connection& ) const;
        bool operator!=( const Connection& ) const;
    private:
        WellCompletion::DirectionEnum direction;
        double center_depth;
        WellCompletion::StateEnum open_state;
        int sat_tableId;
        int m_complnum;
        double m_CF;
        double m_Kh;
        double m_rw;

        std::array<int,3> ijk;
	std::size_t m_seqIndex;

        // related segment number
        // -1 means the completion is not related to segment
        int segment_number = -1;
        double wPi = 1.0;
    };
}

#endif /* COMPLETION_HPP_ */

