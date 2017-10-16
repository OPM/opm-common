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
#include <iostream>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>

namespace Opm {

    Completion::Completion(int i, int j , int k ,
                           int compnum,
                           double depth,
                           WellCompletion::StateEnum state ,
                           const Value<double>& connectionTransmissibilityFactor,
                           const Value<double>& diameter,
                           const Value<double>& skinFactor,
                           const int satTableId,
                           const WellCompletion::DirectionEnum direction)
        : m_i(i), m_j(j), m_k(k),
          m_complnum( compnum ),
          m_diameter(diameter),
          m_connectionTransmissibilityFactor(connectionTransmissibilityFactor),
          m_wellPi(1.0),
          m_skinFactor(skinFactor),
          m_satTableId(satTableId),
          m_state(state),
          m_direction(direction),
          m_center_depth( depth )
    {}

    Completion::Completion(const Completion& oldCompletion, WellCompletion::StateEnum newStatus ) :
        Completion( oldCompletion )
    {
        this->m_state = newStatus;
    }

    Completion::Completion(const Completion& oldCompletion, double wellPi) :
        Completion( oldCompletion )
    {
        if( this->m_wellPi != 0 ) {
            this->m_wellPi *= wellPi;
        } else {
            this->m_wellPi = wellPi;
        }
    }

    Completion::Completion( const Completion& c, int num ) :
        Completion( c )
    {
        this->m_complnum = num;
    }

    Completion::Completion(const Completion& completion_initial, int segment_number, double center_depth)
      : Completion(completion_initial)
    {
        assert(segment_number > 0);
        this->m_segment_number = segment_number;
        this->m_center_depth = center_depth;
    }

    bool Completion::sameCoordinate(const Completion& other) const {
        if ((m_i == other.m_i) &&
            (m_j == other.m_j) &&
            (m_k == other.m_k))
            return true;
        else
            return false;
    }

    bool Completion::sameCoordinate(const int i, const int j, const int k) const {
        if ((m_i == i) && (m_j == j) && (m_k == k)) {
            return true;
        } else {
            return false;
        }
    }


    /**
       This will break up one record and return a pair: <name ,
       [Completion1, Completion2, ... , CompletionN]>. The reason it
       will return a list is that the 'K1 K2' structure is
       disentangled, and each completion is returned separately.
    */

    inline std::vector< Completion >
    fromCOMPDAT( const EclipseGrid& grid,
                 const Eclipse3DProperties& eclipseProperties,
                 const DeckRecord& compdatRecord,
                 const Well& well,
                 int prev_complnum ) {

        std::vector< Completion > completions;

        // We change from eclipse's 1 - n, to a 0 - n-1 solution
        // I and J can be defaulted with 0 or *, in which case they are fetched
        // from the well head
        const auto& itemI = compdatRecord.getItem( "I" );
        const auto defaulted_I = itemI.defaultApplied( 0 ) || itemI.get< int >( 0 ) == 0;
        const int I = !defaulted_I ? itemI.get< int >( 0 ) - 1 : well.getHeadI();

        const auto& itemJ = compdatRecord.getItem( "J" );
        const auto defaulted_J = itemJ.defaultApplied( 0 ) || itemJ.get< int >( 0 ) == 0;
        const int J = !defaulted_J ? itemJ.get< int >( 0 ) - 1 : well.getHeadJ();

        int K1 = compdatRecord.getItem("K1").get< int >(0) - 1;
        int K2 = compdatRecord.getItem("K2").get< int >(0) - 1;
        WellCompletion::StateEnum state = WellCompletion::StateEnumFromString( compdatRecord.getItem("STATE").getTrimmedString(0) );
        Value<double> connectionTransmissibilityFactor("ConnectionTransmissibilityFactor");
        Value<double> diameter("Diameter");
        Value<double> skinFactor("SkinFactor");
        int satTableId;
        const auto& satnum = eclipseProperties.getIntGridProperty("SATNUM");
        bool defaultSatTable = true;
        {
            const auto& connectionTransmissibilityFactorItem = compdatRecord.getItem("CONNECTION_TRANSMISSIBILITY_FACTOR");
            const auto& diameterItem = compdatRecord.getItem("DIAMETER");
            const auto& skinFactorItem = compdatRecord.getItem("SKIN");
            const auto& satTableIdItem = compdatRecord.getItem("SAT_TABLE");

            if (connectionTransmissibilityFactorItem.hasValue(0) && connectionTransmissibilityFactorItem.getSIDouble(0) > 0)
                connectionTransmissibilityFactor.setValue(connectionTransmissibilityFactorItem.getSIDouble(0));

            if (diameterItem.hasValue(0))
                diameter.setValue( diameterItem.getSIDouble(0));

            if (skinFactorItem.hasValue(0))
                skinFactor.setValue( skinFactorItem.get< double >(0));

            if (satTableIdItem.hasValue(0) && satTableIdItem.get < int > (0) > 0)
            {
                satTableId = satTableIdItem.get< int >(0);
                defaultSatTable = false;
            }
        }

        const WellCompletion::DirectionEnum direction = WellCompletion::DirectionEnumFromString(compdatRecord.getItem("DIR").getTrimmedString(0));

        for (int k = K1; k <= K2; k++) {
            if (defaultSatTable)
                satTableId = satnum.iget(grid.getGlobalIndex(I,J,k));

            completions.emplace_back( I, J, k,
                                      int( completions.size() + prev_complnum ) + 1,
                                      grid.getCellDepth( I,J,k ),
                                      state,
                                      connectionTransmissibilityFactor,
                                      diameter,
                                      skinFactor,
                                      satTableId,
                                      direction );
        }

        return completions;
    }

    /*
      Will return a map:

      {
         "WELL1" : [ Completion1 , Completion2 , ... , CompletionN ],
         "WELL2" : [ Completion1 , Completion2 , ... , CompletionN ],
         ...
      }
    */

    std::map< std::string, std::vector< Completion > >
    Completion::fromCOMPDAT( const EclipseGrid& grid ,
                             const Eclipse3DProperties& eclipseProperties,
                             const DeckKeyword& compdatKeyword,
                             const std::vector< const Well* >& wells ) {

        std::map< std::string, std::vector< Completion > > res;
        std::vector< int > prev_compls( wells.size(), 0 );

        for( const auto& record : compdatKeyword ) {

            const auto wellname = record.getItem( "WELL" ).getTrimmedString( 0 );
            const auto name_eq = [&]( const Well* w ) {
                return w->name() == wellname;
            };

            auto well = std::find_if( wells.begin(), wells.end(), name_eq );

            if( well == wells.end() ) continue;

            const auto index = std::distance( wells.begin(), well );
            if( prev_compls[ index ] == 0 ) (*well)->getCompletions().size();

            auto completions = Opm::fromCOMPDAT( grid,
                                                 eclipseProperties,
                                                 record,
                                                 **well,
                                                 prev_compls[ index ] );

            prev_compls[ index ] += completions.size();

            res[ wellname ].insert( res[ wellname ].end(),
                                    std::make_move_iterator( completions.begin() ),
                                    std::make_move_iterator( completions.end() ) );
        }

        return res;
    }

    void Completion::fixDefaultIJ(int wellHeadI , int wellHeadJ) {
        if (m_i < 0)
            m_i = wellHeadI;

        if (m_j < 0)
            m_j = wellHeadJ;
    }

    void Completion::shift_complnum( int shift ) {
        this->m_complnum += shift;
    }

    int Completion::getI() const {
        return m_i;
    }

    int Completion::getJ() const {
        return m_j;
    }

    int Completion::getK() const {
        return m_k;
    }

    int Completion::complnum() const {
        return this->m_complnum;
    }

    WellCompletion::StateEnum Completion::getState() const {
        return m_state;
    }

    double Completion::getConnectionTransmissibilityFactor() const {
        return m_connectionTransmissibilityFactor.getValue();
    }

    double Completion::getDiameter() const {
        return m_diameter.getValue();
    }

    double Completion::getSkinFactor() const {
        return m_skinFactor.getValue();
    }

    int Completion::getSatTableId() const {
        return m_satTableId;
    }

    const Value<double>& Completion::getConnectionTransmissibilityFactorAsValueObject() const {
        return m_connectionTransmissibilityFactor;
    }

    Value<double> Completion::getDiameterAsValueObject() const {
        return m_diameter;
    }

    Value<double> Completion::getSkinFactorAsValueObject() const {
        return m_skinFactor;
    }

    WellCompletion::DirectionEnum Completion::getDirection() const {
        return m_direction;
    }

    double Completion::getWellPi() const {
        return m_wellPi;
    }

    int Completion::getSegmentNumber() const {
        if (!attachedToSegment()) {
            throw std::runtime_error(" the completion is not attached to a segment!\n ");
        }
        return m_segment_number;
    }

    double Completion::getCenterDepth() const {
        return m_center_depth;
    }

    bool Completion::attachedToSegment() const {
        return (m_segment_number > 0);
    }

    bool Completion::operator==( const Completion& rhs ) const {
        return this->m_i == rhs.m_i
            && this->m_j == rhs.m_j
            && this->m_k == rhs.m_k
            && this->m_complnum == rhs.m_complnum
            && this->m_diameter == rhs.m_diameter
            && this->m_connectionTransmissibilityFactor
               == rhs.m_connectionTransmissibilityFactor
            && this->m_wellPi == rhs.m_wellPi
            && this->m_skinFactor == rhs.m_skinFactor
            && this->m_satTableId == rhs.m_satTableId
            && this->m_state == rhs.m_state
            && this->m_direction == rhs.m_direction
            && this->m_segment_number == rhs.m_segment_number
            && this->m_center_depth == rhs.m_center_depth;
    }

    bool Completion::operator!=( const Completion& rhs ) const {
        return !( *this == rhs );
    }
}


