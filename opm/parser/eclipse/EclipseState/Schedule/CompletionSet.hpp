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


#ifndef COMPLETIONSET_HPP_
#define COMPLETIONSET_HPP_

#include <boost/date_time.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>


namespace Opm {

    class CompletionSet {
    public:
        CompletionSet();
        void add(CompletionConstPtr completion);
        size_t size() const;
        CompletionSet * shallowCopy();
        CompletionConstPtr get(size_t index) const;
    private:
        std::vector<CompletionConstPtr> m_completions;
    };

    typedef boost::shared_ptr<CompletionSet> CompletionSetPtr;
    typedef boost::shared_ptr<const CompletionSet> CompletionSetConstPtr;
}



#endif 
