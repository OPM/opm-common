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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/Equil.hpp>

namespace Opm {

    static inline Equil equils( const Deck& deck ) {
        if( !deck.hasKeyword( "EQUIL" ) ) return {};
        return Equil( deck.getKeyword( "EQUIL" ) );
    }

    InitConfig::InitConfig(DeckConstPtr deck) : equil( equils( *deck ) ) {
        m_restartInitiated = false;
        m_restartStep = 0;
        m_restartRootName = "";

        initRestartKW(deck);
    }

    void InitConfig::initRestartKW(DeckConstPtr deck) {
        if (deck->hasKeyword("RESTART")) {
            const auto& restart_kw = deck->getKeyword("RESTART");
            const auto& restart_dataRecord = restart_kw.getRecord(0);
            const auto& restart_rootname_item = restart_dataRecord.getItem(0);
            const std::string restart_rootname_string = restart_rootname_item.get< std::string >(0);

            const auto& restart_report_step_item = restart_dataRecord.getItem(1);
            int restart_report_step_int = restart_report_step_item.get< int >(0);

            const auto& save_item = restart_dataRecord.getItem(2);
            if (save_item.hasValue(0)) {
                throw std::runtime_error("OPM does not support RESTART from a SAVE file, only from RESTART files");
            }

            m_restartInitiated = true;
            m_restartStep = restart_report_step_int;
            m_restartRootName = restart_rootname_string;
        } else if (deck->hasKeyword("SKIPREST"))
            throw std::runtime_error("Error in deck: Can not supply SKIPREST keyword without a preceeding RESTART.");
    }

    bool InitConfig::getRestartInitiated() const {
        return m_restartInitiated;
    }

    int InitConfig::getRestartStep() const {
        return m_restartStep;
    }

    const std::string& InitConfig::getRestartRootName() const {
        return m_restartRootName;
    }

    bool InitConfig::hasEquil() const {
        return !this->equil.empty();
    }

    const Equil& InitConfig::getEquil() const {
        if( !this->hasEquil() )
            throw std::runtime_error( "Error: No 'EQUIL' present" );

        return this->equil;
    }

} //namespace Opm
