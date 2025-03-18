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

#include <opm/common/ErrorMacros.hpp>

#include <opm/input/eclipse/Schedule/RPTKeywordNormalisation.hpp>
#include <opm/input/eclipse/Schedule/RptschedKeywordNormalisation.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <map>
#include <string>
#include <vector>

namespace {
    class AcceptAllMnemonics
    {
    public:
        bool operator()([[maybe_unused]] const std::string& mnemonic) const
        {
            return true;
        }
    };

    class NoIntegerControls
    {
    public:
        Opm::RPTKeywordNormalisation::MnemonicMap
        operator()([[maybe_unused]] const std::vector<int>& controlValues) const
        {
            return {};
        }
    };
}

namespace Opm {

RPTConfig RPTConfig::serializationTestObject()
{
    RPTConfig rptc;
    rptc.m_mnemonics.emplace( "KEY", 100 );
    return rptc;
}

RPTConfig::RPTConfig(const DeckKeyword& keyword)
{
    const auto parseContext = ParseContext{};
    auto errors = ErrorGuard{};

    const auto mnemonics = RPTKeywordNormalisation {
        NoIntegerControls{}, AcceptAllMnemonics{}
    }.normaliseKeyword(keyword, parseContext, errors);

    this->assignMnemonics(mnemonics);
}

RPTConfig::RPTConfig(const DeckKeyword&  keyword,
                     const ParseContext& parseContext,
                     ErrorGuard&         errors,
                     const RPTConfig*    prev)
{
    if (prev != nullptr) {
        this->m_mnemonics = prev->m_mnemonics;
    }

    const auto mnemonics =
        normaliseRptSchedKeyword(keyword, parseContext, errors);

    this->assignMnemonics(mnemonics);
}

bool RPTConfig::operator==(const RPTConfig& other) const
{
    return this->m_mnemonics == other.m_mnemonics;
}

bool RPTConfig::contains(const std::string& key) const
{
    return this->m_mnemonics.find(key) != this->m_mnemonics.end();
}

void RPTConfig::assignMnemonics(const std::map<std::string, int>& mnemonics)
{
    for (const auto& [mnemonic, value] : mnemonics) {
        if (mnemonic == "NOTHING") {
            this->m_mnemonics.clear();
        }
        else {
            this->m_mnemonics.insert_or_assign(mnemonic, value);
        }
    }
}

} // namespace Opm
