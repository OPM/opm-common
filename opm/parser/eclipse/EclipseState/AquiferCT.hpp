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

#ifndef OPM_AQUIFERCT_HPP
#define OPM_AQUIFERCT_HPP

/*
  The AquiferCT which stands for AquiferCarterTracy is a data container object meant to hold the data for the aquifer carter tracy model.
  This includes the logic for parsing as well as the associated tables. It is meant to be used by opm-grid and opm-simulators in order to
  implement the Carter Tracy analytical aquifer model in OPM Flow.
*/
#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/Aqudims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/AqutabTable.hpp>

namespace Opm {

    class TableManager;

    class AquiferCT {
        public:

        struct AQUCT_data{

            AQUCT_data(const DeckRecord& record, const TableManager& tables);

            int aquiferID;
            int inftableID, pvttableID;

            double  phi_aq , // aquifer porosity
                    d0,      // aquifer datum depth
                    C_t ,    // total compressibility
                    r_o ,    // aquifer inner radius
                    k_a ,    // aquifer permeability
                    c1,      // 0.008527 (METRIC, PVT-M); 0.006328 (FIELD); 3.6 (LAB)
                    h ,      // aquifer thickness
                    theta ,  // angle subtended by the aquifer boundary
                    c2 ;     // 6.283 (METRIC, PVT-M); 1.1191 (FIELD); 6.283 (LAB).

            std::pair<bool, double> p0; //Initial aquifer pressure at datum depth, d0
            std::vector<double> td, pi;
            std::vector<int> cell_id;

            bool operator==(const AQUCT_data& other) const;
        };

        AquiferCT(const TableManager& tables, const Deck& deck);
        AquiferCT(const std::vector<AquiferCT::AQUCT_data>& data);

        std::size_t size() const;
        std::vector<AquiferCT::AQUCT_data>::const_iterator begin() const;
        std::vector<AquiferCT::AQUCT_data>::const_iterator end() const;
        const std::vector<AquiferCT::AQUCT_data>& data() const;
        bool operator==(const AquiferCT& other) const;
    private:
        std::vector<AquiferCT::AQUCT_data> m_aquct;
    };
}


#endif
