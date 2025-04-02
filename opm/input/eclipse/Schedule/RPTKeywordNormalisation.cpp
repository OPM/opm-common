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

#include <opm/input/eclipse/Schedule/RPTKeywordNormalisation.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Utility/Functional.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <algorithm>
#include <cctype>
#include <functional>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace {

    bool isInteger(const std::string& x)
    {
        auto is_digit = [](char c) { return std::isdigit(c); };

        return !x.empty()
            && ((x.front() == '-') || is_digit(x.front()))
            && std::all_of(x.begin() + 1, x.end(), is_digit);
    }

    std::vector<int>
    getIntegerControlValues(const std::vector<std::string>& deckItems)
    {
        auto controlValues = std::vector<int>{};
        controlValues.reserve(deckItems.size());

        std::transform(deckItems.begin(), deckItems.end(),
                       std::back_inserter(controlValues),
                       [](const std::string& controlItem)
                       { return std::stoi(controlItem); });

        return controlValues;
    }

    std::pair<bool, bool>
    classifyRptKeywordSpecification(const std::vector<std::string>& deckItems)
    {
        return {
            std::any_of(deckItems.begin(), deckItems.end(), isInteger),
            ! std::all_of(deckItems.begin(), deckItems.end(), isInteger)
        };
    }

    void recordUnknownMnemonic(const std::string&          mnemonic,
                               const Opm::KeywordLocation& location,
                               const Opm::ParseContext&    parseContext,
                               Opm::ErrorGuard&            errors)
    {
        const auto msg_fmt =
            fmt::format("Error in keyword {{keyword}}, "
                        "unrecognized mnemonic {}\n"
                        "In {{file}} line {{line}}.", mnemonic);

        parseContext.handleError(Opm::ParseContext::RPT_UNKNOWN_MNEMONIC,
                                 msg_fmt, location, errors);
    }

    int parseMnemonicValue(const std::string&           item,
                           const std::string::size_type sepPos)
    {
        if (sepPos == std::string::npos) {
            return 1;
        }

        const auto valuePos =
            item.find_first_not_of("= ", sepPos);

        return (valuePos != std::string::npos)
            ? std::stoi(item.substr(valuePos))
            : 1;
    }

} // Anonymous namespace

Opm::RPTKeywordNormalisation::MnemonicMap
Opm::RPTKeywordNormalisation::normaliseKeyword(const DeckKeyword&  kw,
                                               const ParseContext& parseContext,
                                               ErrorGuard&         errors) const
{
    const auto [hasIntegerControls, hasMnemonicControls] =
        classifyRptKeywordSpecification(kw.getStringData());

    if (! (hasIntegerControls || hasMnemonicControls)) {
        // Neither regular mnemonics nor integer controls.  This is an empty
        // keyword.
        return {};
    }

    if (! hasMnemonicControls) {
        // Integer controls only.  Defer processing to client's integer
        // control handler.
        return this->integerControlHandler_
            (getIntegerControlValues(kw.getStringData()));
    }

    if (! hasIntegerControls) {
        // Regular mnemonics only.  Handle in normal way.
        return this->parseMnemonics(kw.getStringData(),
                                    kw.location(),
                                    parseContext, errors);
    }

    // If we get here, we have both regular mnemonics *and* integer
    // controls.  This is strictly speaking an error, but we sometimes see
    // input which happens to have blanks on either side of an equals sign,
    // e.g.,
    //
    //   RPTRST
    //     BASIC = 2 /
    //
    // Depending on RPT_MIXED_STYLE, we heuristically interpret the
    // specification as a set of mnemonics.
    const auto msg = std::string {
        "Keyword {keyword} mixes mnemonics and integer controls.\n"
        "This is not permitted.\n"
        "In {file} line {line}."
    };

    parseContext.handleError(ParseContext::RPT_MIXED_STYLE,
                             msg, kw.location(), errors);

    return this->parseMixedStyle(kw, parseContext, errors);
}

// ===========================================================================
// Private member functions of class RPTKeywordNormalisation below
// ===========================================================================

Opm::RPTKeywordNormalisation::MnemonicMap
Opm::RPTKeywordNormalisation::
parseMnemonics(const std::vector<std::string>& deckItems,
               const KeywordLocation&          location,
               const ParseContext&             parseContext,
               ErrorGuard&                     errors) const
{
    auto mnemonics = MnemonicMap{};

    for (const auto& item : deckItems) {
        const auto sepPos   = item.find_first_of( "= " );
        const auto mnemonic = item.substr(0, sepPos);

        if (! this->isMnemonic_(mnemonic)) {
            recordUnknownMnemonic(mnemonic, location, parseContext, errors);
            continue;
        }

        mnemonics.emplace_back(mnemonic, parseMnemonicValue(item, sepPos));
    }

    return mnemonics;
}

Opm::RPTKeywordNormalisation::MnemonicMap
Opm::RPTKeywordNormalisation::
parseMixedStyle(const DeckKeyword&  kw,
                const ParseContext& parseContext,
                ErrorGuard&         errors) const
{
    const auto& deckItems = kw.getStringData();

    auto items = std::vector<std::string>{};
    items.reserve(deckItems.size()); // Best estimate.

    for (const auto& item : deckItems) {
        if (! isInteger(item)) {
            // Regular mnemonic or equals sign.
            items.push_back(item);
            continue;
        }

        // If we get here, then 'item' is an integer.  This is okay if the
        // previous two tokens were exactly
        //
        //   "MNEMONIC"  (e.g., 'BASIC')
        //   "="
        //
        // Otherwise, we have an unrecoverable parse error.

        if ((items.size() < 2) || (items.back() != "=")) {
            throw OpmInputError {
                "Problem processing {keyword}\n"
                "In {file} line {line}.",
                kw.location()
            };
        }

        // Drop '='
        items.pop_back();

        // Make last item be "MNEMONIC=INT".
        items.back() += "=" + item;
    }

    return this->parseMnemonics(items, kw.location(), parseContext, errors);
}
