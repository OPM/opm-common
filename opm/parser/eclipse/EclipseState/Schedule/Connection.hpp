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
                   WellCompletion::StateEnum state ,
                   const Value<double>& connectionTransmissibilityFactor,
                   const Value<double>& diameter,
                   const Value<double>& skinFactor,
                   const Value<double>& Kh,
                   const int satTableId,
                   const WellCompletion::DirectionEnum direction);


        bool sameCoordinate(const int i, const int j, const int k) const;
        int getI() const;
        int getJ() const;
        int getK() const;
        double getConnectionTransmissibilityFactor() const;
        double getDiameter() const;
        double getSkinFactor() const;
        bool attachedToSegment() const;
        const Value<double>& getConnectionTransmissibilityFactorAsValueObject() const;
        const Value<double>& getEffectiveKhAsValueObject() const;

        bool operator==( const Connection& ) const;
        bool operator!=( const Connection& ) const;

        WellCompletion::DirectionEnum dir;
        double center_depth;
        WellCompletion::StateEnum state;
        int sat_tableId;
        int complnum;

    private:
        std::array<int,3> ijk;
        Value<double> m_diameter;
        Value<double> m_connectionTransmissibilityFactor;
        Value<double> m_skinFactor;
        Value<double> m_Kh;

    public:
        // related segment number
        // -1 means the completion is not related to segment
        int segment_number = -1;
        double wellPi = 1.0;
    };
}



#endif /* COMPLETION_HPP_ */
