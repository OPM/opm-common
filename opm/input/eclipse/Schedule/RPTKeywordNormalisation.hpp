/*
  Copyright 2025 Equinor ASA.

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

#ifndef OPM_RPT_KEYWORD_NORMALISATION_HPP_INCLUDED
#define OPM_RPT_KEYWORD_NORMALISATION_HPP_INCLUDED

#include <functional>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Opm {
    class DeckKeyword;
    class ErrorGuard;
    class KeywordLocation;
    class ParseContext;
} // namespace Opm

namespace Opm {

class RPTKeywordNormalisation
{
public:
    using MnemonicMap = std::map<std::string, int>;

    using IntegerControlHandler = std::function<MnemonicMap(const std::vector<int>&)>;
    using MnemonicPredicate = std::function<bool(const std::string&)>;

    explicit RPTKeywordNormalisation(IntegerControlHandler integerControlHandler,
                                     MnemonicPredicate     isMnemonic)
        : integerControlHandler_ { std::move(integerControlHandler) }
        , isMnemonic_            { std::move(isMnemonic) }
    {}

    MnemonicMap normaliseKeyword(const DeckKeyword&  kw,
                                 const ParseContext& parseContext,
                                 ErrorGuard&         errors) const;

private:
    IntegerControlHandler integerControlHandler_{};
    MnemonicPredicate     isMnemonic_{};

    MnemonicMap parseMnemonics(const std::vector<std::string>& deckItems,
                               const KeywordLocation&          location,
                               const ParseContext&             parseContext,
                               ErrorGuard&                     errors) const;

    MnemonicMap parseMixedStyle(const DeckKeyword&  kw,
                                const ParseContext& parseContext,
                                ErrorGuard&         errors) const;
};

} // namespace Opm

#endif // OPM_RPT_KEYWORD_NORMALISATION_HPP_INCLUDED
