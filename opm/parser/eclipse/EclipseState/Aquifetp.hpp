/*
  Copyright (C) 2017 TNO

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

#ifndef OPM_AQUIFERFETP_HPP
#define OPM_AQUIFERFETP_HPP

/*
  The Aquiferfetp which stands for AquiferFetkovich is a data container object meant to hold the data for the fetkovich aquifer model.
  This includes the logic for parsing as well as the associated tables. It is meant to be used by opm-grid and opm-simulators in order to
  implement the Fetkovich analytical aquifer model in OPM Flow.
*/

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Aqudims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <boost/concept_check.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

    class Aquifetp {
        public:

            struct AQUFETP_data{

                    // Aquifer ID
                    int aquiferID;
                    // Table IDs
                    int inftableID, pvttableID;
                    std::vector<int> cell_id;
                    // Variables constants
                    double  J, // Specified Productivity Index
			  rho, // water density in the aquifer
			  C_t, // total rock compressibility
			   V0, // initial volume of water in aquifer
			   p0, // initial pressure of water in aquifer
			   d0; // aquifer datum depth
		    
            };

            Aquifetp(const Deck& deck);

            const std::vector<Aquifetp::AQUFETP_data>& getAquifers() const;
            int getAqPvtTabID(size_t aquiferIndex);
    
        private:
  
            std::vector<Aquifetp::AQUFETP_data> m_aqufetp;

    };
}


#endif
