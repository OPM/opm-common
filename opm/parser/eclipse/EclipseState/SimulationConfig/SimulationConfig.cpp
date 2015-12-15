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


#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>



/*
  The internalization of the CPR keyword has been temporarily
  disabled, suddenly decks with 'CPR' in the summary section turned
  up. Keywords with section aware keyword semantics is currently not
  handled by the parser.

  When the CPR is added again the following keyword configuration must
  be added:

    {"name" : "CPR" , "sections" : ["RUNSPEC"], "size": 1 }

*/


namespace Opm {

    SimulationConfig::SimulationConfig(const ParseMode& parseMode , DeckConstPtr deck, std::shared_ptr<GridProperties<int>> gridProperties) :
        m_useCPR(false),
        m_DISGAS(false),
        m_VAPOIL(false)
    {
        if (Section::hasRUNSPEC(deck)) {
            const RUNSPECSection runspec(deck);
            if (runspec.hasKeyword<ParserKeywords::CPR>()) {
                std::shared_ptr<const DeckKeyword> cpr = runspec.getKeyword<ParserKeywords::CPR>();
                if (cpr->size() > 0)
                    throw std::invalid_argument("ERROR: In the RUNSPEC section the CPR keyword should EXACTLY one empty record.");

                m_useCPR = true;
            }
            if (runspec.hasKeyword<ParserKeywords::DISGAS>()) {
                m_DISGAS = true;
            }
            if (runspec.hasKeyword<ParserKeywords::VAPOIL>()) {
                m_VAPOIL = true;
            }
        }

        initThresholdPressure(parseMode , deck, gridProperties);
    }


    void SimulationConfig::initThresholdPressure(const ParseMode& parseMode, DeckConstPtr deck, std::shared_ptr<GridProperties<int>> gridProperties) {
        m_ThresholdPressure = std::make_shared<const ThresholdPressure>(parseMode , deck, gridProperties);
    }


    std::shared_ptr<const ThresholdPressure> SimulationConfig::getThresholdPressure() const {
        return m_ThresholdPressure;
    }


    bool SimulationConfig::hasThresholdPressure() const {
        return m_ThresholdPressure->size() > 0;
    }

    bool SimulationConfig::useCPR() const {
        return m_useCPR;
    }

    bool SimulationConfig::hasDISGAS() const {
        return m_DISGAS;
    }

    bool SimulationConfig::hasVAPOIL() const {
        return m_VAPOIL;
    }

} //namespace Opm
