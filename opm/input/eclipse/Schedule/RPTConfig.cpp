/*
  Copyright 2020 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/RPTConfig.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/common/ErrorMacros.hpp>

namespace {

    std::pair<std::string, unsigned> parse_mnemonic(const std::string& mnemonic) {
        const auto pivot { mnemonic.find('=') } ;

        if (pivot == std::string::npos) {
            return { mnemonic, 1 } ;
        } else {
            const auto int_value { std::stoi(mnemonic.substr(pivot + 1)) } ;

            if (!(int_value >= 0)) {
                OPM_THROW(std::invalid_argument, "RPTSCHED - " + mnemonic + " - mnemonic value must be an integer >= 0");
            }

            return { mnemonic.substr(0, pivot), int_value } ;
        }
    }

}

namespace Opm {

RPTConfig RPTConfig::serializationTestObject() {
    RPTConfig rptc;
    rptc.m_mnemonics.emplace( "KEY", 100 );
    return rptc;
}


RPTConfig::RPTConfig(const DeckKeyword& keyword, const RPTConfig* prev) {
    if (prev)
        this->m_mnemonics = prev->m_mnemonics;

    const auto& mnemonics { keyword.getStringData() } ;
    for (const auto& mnemonic : mnemonics) {
        if (mnemonic == "NOTHING")
            this->m_mnemonics.clear();
        else {
            const auto key_value = parse_mnemonic(mnemonic);
            this->m_mnemonics.insert_or_assign(key_value.first, key_value.second);
        }
    }
}


bool RPTConfig::operator==(const RPTConfig& other) const {
    return this->m_mnemonics == other.m_mnemonics;
}

bool RPTConfig::contains(const std::string& key) const {
    return this->m_mnemonics.find(key) != this->m_mnemonics.end();
}

}
