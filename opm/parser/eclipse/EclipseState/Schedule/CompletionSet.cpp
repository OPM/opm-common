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

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>

namespace Opm {

    CompletionSet::CompletionSet() {}

    
    size_t CompletionSet::size() const {
        return m_completions.size();
    }

    
    CompletionConstPtr CompletionSet::get(size_t index) {
        if (index >= m_completions.size())
            throw std::range_error("Out of bounds");
        
        return m_completions[index];
    }
    

    void CompletionSet::add(CompletionConstPtr completion) {
        bool inserted = false;

        for (size_t ic = 0; ic < m_completions.size(); ic++) {
            CompletionConstPtr current = m_completions[ic];
            if (current->sameCoordinate( *completion )) {
                m_completions[ic] = completion;
                inserted = true;
            }
        }
        
        if (!inserted)
            m_completions.push_back( completion );
    }


    CompletionSet CompletionSet::shallowCopy() {
        CompletionSet copy;
        for (size_t ic = 0; ic < m_completions.size(); ic++) {
            CompletionConstPtr completion = m_completions[ic];
            copy.m_completions.push_back( completion );
        }
        return copy;
    }


}
