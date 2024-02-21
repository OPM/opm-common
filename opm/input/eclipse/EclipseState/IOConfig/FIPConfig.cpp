/*
  Copyright 2023 SINTEF Digital

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

#include <opm/input/eclipse/EclipseState/IOConfig/FIPConfig.hpp>

#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>

#include <opm/input/eclipse/Schedule/RPTConfig.hpp>

#include <vector>

namespace Opm {

FIPConfig::FIPConfig(const Deck& deck)
{
    SOLUTIONSection solution_section(deck);
    if (solution_section.hasKeyword<ParserKeywords::RPTSOL>()) {
        this->parseRPT(solution_section.get<ParserKeywords::RPTSOL>().back());
    }
}

FIPConfig::FIPConfig(const DeckKeyword& keyword)
    : FIPConfig(RPTConfig(keyword))
{
}

FIPConfig::FIPConfig(const RPTConfig& rptConfig)
{
    this->parseRPT(rptConfig);
}

void FIPConfig::parseRPT(const RPTConfig& rptConfig)
{
    auto parseFlags = [this](const std::vector<int>& flags,
                             const unsigned value)
    {
        for (size_t i = 0; i < flags.size(); ++i) {
            if (value > i) {
                m_flags.set(flags[i]);
            }
        }
    };

    for (const auto& mnemonic : rptConfig) {
        if (mnemonic.first == "FIP") {
            parseFlags({static_cast<int>(OutputField::FIELD),
                        static_cast<int>(OutputField::FIPNUM),
                        static_cast<int>(OutputField::FIP)}, mnemonic.second);
        } else if (mnemonic.first == "FIPFOAM") {
            parseFlags({static_cast<int>(OutputField::FOAM_FIELD),
                        static_cast<int>(OutputField::FOAM_REGION)}, mnemonic.second);
        } else if (mnemonic.first == "FIPPLY") {
            parseFlags({static_cast<int>(OutputField::POLYMER_FIELD),
                        static_cast<int>(OutputField::POLYMER_REGION)}, mnemonic.second);
        } else if (mnemonic.first == "FIPSOL") {
            parseFlags({static_cast<int>(OutputField::SOLVENT_FIELD),
                        static_cast<int>(OutputField::SOLVENT_REGION)}, mnemonic.second);
        } else if (mnemonic.first == "FIPSURF") {
            parseFlags({static_cast<int>(OutputField::SURF_FIELD),
                        static_cast<int>(OutputField::SURF_REGION)}, mnemonic.second);
        } else if (mnemonic.first == "FIPHEAT" || mnemonic.first == "FIPTEMP") {
            parseFlags({static_cast<int>(OutputField::TEMPERATURE_FIELD),
                        static_cast<int>(OutputField::TEMPERATURE_REGION)}, mnemonic.second);
        } else if (mnemonic.first == "FIPTR") {
            parseFlags({static_cast<int>(OutputField::TRACER_FIELD),
                        static_cast<int>(OutputField::TRACER_REGION)}, mnemonic.second);
        } else if (mnemonic.first == "FIPRESV") {
            m_flags.set(static_cast<int>(OutputField::RESV));
        } else if (mnemonic.first == "FIPVE") {
            m_flags.set(static_cast<int>(OutputField::VE));
        }
    }
}

FIPConfig FIPConfig::serializationTestObject()
{
    FIPConfig result;
    result.m_flags.set(static_cast<int>(OutputField::FIELD));
    result.m_flags.set(static_cast<int>(OutputField::FIP));
    result.m_flags.set(static_cast<int>(OutputField::RESV));

    return result;
}

bool FIPConfig::output(OutputField field) const
{
    return m_flags.test(static_cast<int>(field));
}

bool FIPConfig::operator==(const FIPConfig& rhs) const
{
    return this->m_flags == rhs.m_flags;
}

} //namespace Opm
