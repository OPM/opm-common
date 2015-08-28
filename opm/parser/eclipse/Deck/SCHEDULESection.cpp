/*
  Copyright 2015 Statoil ASA.

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


#include <opm/parser/eclipse/Deck/SCHEDULESection.hpp>

namespace Opm {

    SCHEDULESection::SCHEDULESection(DeckConstPtr deck) : Section(deck, "SCHEDULE") {
        populateDeckTimeSteps();
    }


    DeckTimeStepConstPtr SCHEDULESection::getDeckTimeStep(size_t timestep) const {
        if (timestep < m_decktimesteps.size()) {
            return m_decktimesteps[timestep];
        } else {
            throw std::out_of_range("No DeckTimeStep in ScheduleSection for timestep " + timestep);
        }
    }


    void SCHEDULESection::populateDeckTimeSteps() {
        DeckTimeStepPtr currentTimeStep = std::make_shared<DeckTimeStep>();

        for (auto iter = begin(); iter != end(); ++iter) {  //Loop keywords in schedule section
           auto keyword = *iter;
           if ((keyword->name() == "TSTEP") || (keyword->name() == "DATES")) {
               for (auto key_iter = keyword->begin(); key_iter != keyword->end(); ++key_iter) {
                   m_decktimesteps.push_back(currentTimeStep);
                   currentTimeStep = std::make_shared<DeckTimeStep>();
               }
           } else {
                 currentTimeStep->addKeyword(keyword);
           }
        }
        //push last step
        m_decktimesteps.push_back(currentTimeStep);
    }
}
