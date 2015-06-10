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

#include <memory>
#include <string>
#include <boost/date_time.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>


namespace Opm {

    class Completion {
    public:
        Completion(int i, int j , int k , WellCompletion::StateEnum state ,
                   const Value<double>& connectionTransmissibilityFactor,
                   const Value<double>& diameter,
                   const Value<double>& skinFactor,
                   const WellCompletion::DirectionEnum direction = WellCompletion::DirectionEnum::Z);

        Completion(std::shared_ptr<const Completion> oldCompletion, WellCompletion::StateEnum newStatus);
        Completion(std::shared_ptr<const Completion> oldCompletion, double wellPi);

        bool sameCoordinate(const Completion& other) const;
        int getI() const;
        int getJ() const;
        int getK() const;
        WellCompletion::StateEnum getState() const;
        double getConnectionTransmissibilityFactor() const;
        double getWellPi() const;
        const Value<double>& getConnectionTransmissibilityFactorAsValueObject() const;
        double getDiameter() const;
        double getSkinFactor() const;
        void   fixDefaultIJ(int wellHeadI , int wellHeadJ);

        WellCompletion::DirectionEnum getDirection() const;

        static std::map<std::string , std::vector<std::shared_ptr<Completion> > >  completionsFromCOMPDATKeyword( DeckKeywordConstPtr compdatKeyword );
        static std::pair<std::string , std::vector<std::shared_ptr<Completion> > > completionsFromCOMPDATRecord( DeckRecordConstPtr compdatRecord );

    private:
        int m_i, m_j, m_k;
        Value<double> m_diameter;
        Value<double> m_connectionTransmissibilityFactor;
        double m_wellPi;
        Value<double> m_skinFactor;
        WellCompletion::StateEnum m_state;
        WellCompletion::DirectionEnum m_direction;
        Value<double> getDiameterAsValueObject() const;
        Value<double> getSkinFactorAsValueObject() const;

    };

    typedef std::shared_ptr<Completion> CompletionPtr;
    typedef std::shared_ptr<const Completion> CompletionConstPtr;
}



#endif /* COMPLETION_HPP_ */
