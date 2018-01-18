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

#ifndef OPM_AQUANCON_HPP
#define OPM_AQUANCON_HPP

/*
  Aquancon is a data container object meant to hold the data from the AQUANCON keyword.
  This also includes the logic for parsing and connections to grid cells. It is meant to be used by opm-grid and opm-simulators in order to
  implement the analytical aquifer models in OPM Flow.
*/

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

namespace Opm {
    namespace{
        struct AquanconRecord;
        }

    class Aquancon {
        public:

            struct AquanconOutput{
                int aquiferID;
                std::vector<size_t> global_index;
                std::vector<double> influx_coeff; // Size = size(global_index)
                std::vector<double> influx_multiplier; // Size = size(global_index)
                std::vector<int> reservoir_face_dir; // Size = size(global_index)
            };

            Aquancon(const EclipseGrid& grid, const Deck& deck);

            const std::vector<Aquancon::AquanconOutput>& getAquOutput() const;
    
        private:

            void logic_application(std::vector<Aquancon::AquanconOutput>& output_vector);

            void collate_function(std::vector<Aquancon::AquanconOutput>& output_vector, 
                                  std::vector<Opm::AquanconRecord>& m_aqurecord, 
                                  std::vector<int> m_aquiferID_per_record, int m_maxAquID);

            void convert_record_id_to_aquifer_id(std::vector<int>& record_indices_matching_id, int i,
                                                 std::vector<int> m_aquiferID_per_record);

            std::vector<Aquancon::AquanconOutput> m_aquoutput;
    };
}

#endif
