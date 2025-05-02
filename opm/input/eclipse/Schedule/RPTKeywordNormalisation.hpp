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

/// Normalise disparate input sources into sequence of report keyword
/// mnemonics and associate values.
class RPTKeywordNormalisation
{
public:
    /// Mnemonic sequence.  Preserves input ordering.
    using MnemonicMap = std::vector<std::pair<std::string, int>>;

    /// Callback for translating a sequence of integer controls into a
    /// sequence of mnemonics.
    using IntegerControlHandler = std::function<MnemonicMap(const std::vector<int>&)>;

    /// Mnemonic validity predicate.
    using MnemonicPredicate = std::function<bool(const std::string&)>;

    /// Constructor.
    ///
    /// \param[in] integerControlHandler Callback for translating a sequence
    /// of integer controls, typically from the RPTSCHED, RPTRST, or RPTSOL
    /// keywords, into a sequence of mnemonics.  It is the caller's
    /// responsibility to provide a callback that matches the current report
    /// keyword context.
    ///
    /// \param[in] isMnemonic Callback for checking the validity of a report
    /// mnemonic string.
    explicit RPTKeywordNormalisation(IntegerControlHandler integerControlHandler,
                                     MnemonicPredicate     isMnemonic)
        : integerControlHandler_ { std::move(integerControlHandler) }
        , isMnemonic_            { std::move(isMnemonic) }
    {}

    /// Normalise report keyword specification into sequence of mnemonics
    /// and associate integer values.
    ///
    /// \param[in] kw Report keyword specification, typically from RPTSCHED,
    /// RPTRST, or RPTSOL.
    ///
    /// \param[in] parseContext Error handling controls.
    ///
    /// \param[in,out] errors Collection of parse errors encountered thus
    /// far.  Behaviour controlled by \p parseContext.
    ///
    /// \return Sequence of mnemonics and associate integer values.
    MnemonicMap normaliseKeyword(const DeckKeyword&  kw,
                                 const ParseContext& parseContext,
                                 ErrorGuard&         errors) const;

private:
    /// User-controlled callback for translating sequences of integer
    /// controls into sequences of mnemonics.
    IntegerControlHandler integerControlHandler_{};

    /// User-controlled callback for checking validity of report mnemonic
    /// string.
    MnemonicPredicate isMnemonic_{};

    /// Normalise sequence of mnemonics.
    ///
    /// Mnemonic values are one (1) unless the mnemonic uses assignment
    /// syntax (e.g., BASIC=3).
    ///
    /// \param[in] deckItems Sequence of mnemonics, possibly including value
    /// assignment.
    ///
    /// \param[in] location Input location of the keyword.  Used for
    /// diagnostic generation.
    ///
    /// \param[in] parseContext Error handling controls.
    ///
    /// \param[in,out] errors Collection of parse errors encountered thus
    /// far.  Behaviour controlled by \p parseContext.
    ///
    /// \return Sequence of mnemonics and associate integer values.
    MnemonicMap parseMnemonics(const std::vector<std::string>& deckItems,
                               const KeywordLocation&          location,
                               const ParseContext&             parseContext,
                               ErrorGuard&                     errors) const;

    /// Normalise sequence of mnemonics.
    ///
    /// Input sequence contains a mix of mnemonic strings and integer
    /// controls.  This is strictly speaking an error, but we sometimes see
    /// inputs which happen to have blanks on either side of an equals sign,
    /// e.g.,
    ///
    ///   RPTRST
    ///     BASIC = 2 /
    ///
    /// Depending on RPT_MIXED_STYLE, we heuristically interpret the
    /// specification as a set of mnemonics.
    ///
    /// \param[in] kw Report keyword specification, typically from RPTSCHED,
    /// RPTRST, or RPTSOL.
    ///
    /// \param[in] parseContext Error handling controls.
    ///
    /// \param[in,out] errors Collection of parse errors encountered thus
    /// far.  Behaviour controlled by \p parseContext.
    ///
    /// \return Sequence of mnemonics and associate integer values.
    MnemonicMap parseMixedStyle(const DeckKeyword&  kw,
                                const ParseContext& parseContext,
                                ErrorGuard&         errors) const;
};

} // namespace Opm

#endif // OPM_RPT_KEYWORD_NORMALISATION_HPP_INCLUDED
