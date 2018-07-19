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

#include <iostream>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/Equil.hpp>

#include <opm/parser/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp>

namespace Opm {

    static inline Equil equils( const Deck& deck ) {
        if( !deck.hasKeyword<ParserKeywords::EQUIL>( ) ) return {};
        return Equil( deck.getKeyword<ParserKeywords::EQUIL>(  ) );
    }

    InitConfig::InitConfig(const Deck& deck) : equil(equils(deck)) {
        if( !deck.hasKeyword( "RESTART" ) ) {
            if( deck.hasKeyword( "SKIPREST" ) ) {
                std::cout << "Deck has SKIPREST, but no RESTART. "
                          << "Ignoring SKIPREST." << std::endl;
            }

            return;
        }

        const auto& record = deck.getKeyword( "RESTART" ).getRecord(0);
        const auto& save_item = record.getItem(2);

        if( save_item.hasValue( 0 ) ) {
            throw std::runtime_error(
                    "OPM does not support RESTART from a SAVE file, "
                    "only from RESTART files");
        }

        int step = record.getItem( 1 ).get< int >(0);
        const std::string& root = record.getItem( 0 ).get< std::string >( 0 );
        const std::string& input_path = deck.getInputPath();

        if (root[0] == '/' || input_path.empty())
            this->setRestart(root, step);
        else
            this->setRestart( input_path + "/" + root, step );
    }

    void InitConfig::setRestart( const std::string& root, int step) {
        m_restartRequested = true;
        m_restartStep = step;
        m_restartRootName = root;
    }

    bool InitConfig::restartRequested() const {
        return m_restartRequested;
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
