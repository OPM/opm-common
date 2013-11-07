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

#include <boost/date_time.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>


namespace Opm {

    class Completion {
    public:
        Completion(int i, int j , int k , CompletionStateEnum state , double CF);
        bool sameCoordinate(const Completion& other) const;
        int getI() const;
        int getJ() const;
        int getK() const;
        CompletionStateEnum getState() const;
        double getCF() const;

        static std::map<std::string , std::vector<boost::shared_ptr<const Completion> > >  completionsFromCOMPDATKeyword( DeckKeywordConstPtr compdatKeyword );
        static std::pair<std::string , std::vector<boost::shared_ptr<const Completion> > > completionsFromCOMPDATRecord( DeckRecordConstPtr compdatRecord );
        
    private:
        int m_i, m_j, m_k;
        double m_CF;
        CompletionStateEnum m_state;
    };

    typedef boost::shared_ptr<Completion> CompletionPtr;
    typedef boost::shared_ptr<const Completion> CompletionConstPtr;
}



#endif /* TIMEMAP_HPP_ */
