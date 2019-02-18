/*
  Copyright 2018  Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/U.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>

namespace Opm {

    /*
      The UDQDIMS keyword contains a long list MAX_XXXX items which probably
      originate in an ECLIPSE implementation detail. Our implementation does not
      require these max values, we therefor ignore them completely.
    */
    UDQParams::UDQParams() :
        reseed_rng(false),
        random_seed(ParserKeywords::UDQPARAM::RANDOM_SEED::defaultValue),
        value_range(ParserKeywords::UDQPARAM::RANGE::defaultValue),
        undefined_value(ParserKeywords::UDQPARAM::UNDEFINED_VALUE::defaultValue),
        cmp_eps(ParserKeywords::UDQPARAM::CMP_EPSILON::defaultValue)
    {}


    UDQParams::UDQParams(const Deck& deck) :
        UDQParams()
    {
        if (deck.hasKeyword("UDQDIMS")) {
            const auto& record = deck.getKeyword("UDQDIMS").getRecord(0);
            const auto& item = record.getItem("RESTART_NEW_SEED");
            const auto& bool_string = item.get<std::string>(0);

            reseed_rng = (std::toupper(bool_string[0]) == 'Y');
        }

        if (deck.hasKeyword("UDQPARAM")) {
            const auto& record = deck.getKeyword("UDQPARAM").getRecord(0);
            random_seed = record.getItem("RANDOM_SEED").get<int>(0);
            value_range = record.getItem("RANGE").get<double>(0);
            undefined_value = record.getItem("UNDEFINED_VALUE").get<double>(0);
            cmp_eps = record.getItem("CMP_EPSILON").get<double>(0);
        }
    }


    bool UDQParams::reseedRNG() const noexcept {
        return this->reseed_rng;
    }

    int UDQParams::randomSeed() const noexcept {
        return this->random_seed;
    }

    double UDQParams::range() const noexcept {
        return this->value_range;
    }

    double UDQParams::undefinedValue() const noexcept {
        return this->undefined_value;
    }

    double UDQParams::cmpEpsilon() const noexcept {
        return this->cmp_eps;
    }
}
