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

#include <memory>

#include <boost/date_time.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>

namespace Opm {

    class CompletionSet {
    public:
        CompletionSet();
        void add(CompletionConstPtr completion);
        size_t size() const;
        CompletionSet * shallowCopy() const;
        CompletionConstPtr get(size_t index) const;
        bool allCompletionsShut() const;
        /// Order completions irrespective of input order.
        /// The algorithm used is the following:
        ///     1. The completion nearest to the given (well_i, well_j)
        ///        coordinates in terms of the completion's (i, j) is chosen
        ///        to be the first completion. If non-unique, choose one with
        ///        lowest z-depth (shallowest).
        ///     2. Choose next completion to be nearest to current in (i, j) sense.
        ///        If non-unique choose closest in z-depth (not logical cartesian k).
        ///
        /// \param[in] well_i  logical cartesian i-coordinate of well head
        /// \param[in] well_j  logical cartesian j-coordinate of well head
        /// \param[in] grid    EclipseGrid object, used for cell depths
        void orderCompletions(const size_t well_i, const size_t well_j, EclipseGridConstPtr grid);
    private:
        std::vector<CompletionConstPtr> m_completions;
        /// Helper for orderCompletions.
        size_t findClosestCompletion(const int oi, const int oj, EclipseGridConstPtr grid,
                                     const double oz, const size_t start_pos);
    };

    typedef std::shared_ptr<CompletionSet> CompletionSetPtr;
    typedef std::shared_ptr<const CompletionSet> CompletionSetConstPtr;
}



#endif
