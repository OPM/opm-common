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
#include <boost/concept_check.hpp>

#include <iostream>

namespace Opm {

    class Aquancon {
        public:

            struct AquanconOutput{
                int aquiferID;
                std::vector<size_t> global_index;
                std::vector<double> influx_coeff; // Size = size(global_index)
                std::vector<double> influx_multiplier; // Size = size(global_index)
                std::vector<std::string> reservoir_face_dir; // Size = size(global_index)
            };

            Aquancon(const EclipseGrid& grid, const Deck& deck);

            const std::vector<Aquancon::AquanconOutput>& getAquOutput() const;
    
        private:

            struct AquanconRecord{
                    // Grid cell box definition to connect aquifer
                    int i1, i2, j1, j2, k1, k2;

                    std::vector<size_t> global_index_per_record;

                    // Variables constants
                    std::vector<double>  influx_coeff_per_record,  //Aquifer influx coefficient
                                         influx_mult_per_record;   //Aquifer influx coefficient Multiplier       
                    // Cell face to connect aquifer to        
                    std::vector<std::string> face_per_record;           

            };

            void logic_application(std::vector<Aquancon::AquanconOutput>& output_vector);

            void collate_function(std::vector<Aquancon::AquanconOutput>& output_vector);

            void convert_record_id_to_aquifer_id(std::vector<int>& record_indices_matching_id, int i);

            std::vector<Aquancon::AquanconOutput> m_aquoutput;

            std::vector<Aquancon::AquanconRecord> m_aqurecord;

            // Aquifer ID
            std::vector<int> m_aquiferID_per_record;
            int m_maxAquID = 0;
    };
}

#endif
