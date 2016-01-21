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

#ifndef SCHEDULESECTION_HPP
#define SCHEDULESECTION_HPP


#include <string>
#include <memory>

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Deck/DeckTimeStep.hpp>

namespace Opm {

    class SCHEDULESection : public Section {

    public:
        SCHEDULESection(DeckConstPtr deck);
        DeckTimeStepConstPtr getDeckTimeStep(size_t timestep) const;


    private:
        void populateDeckTimeSteps();
        std::vector<DeckTimeStepPtr> m_decktimesteps;

    };
}

#endif // SCHEDULESECTION_HPP
