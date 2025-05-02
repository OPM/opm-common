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

#ifndef OPM_RPTSHCED_KEYWORD_NORMALISATION_HPP_INCLUDED
#define OPM_RPTSHCED_KEYWORD_NORMALISATION_HPP_INCLUDED

#include <opm/input/eclipse/Schedule/RPTKeywordNormalisation.hpp>

namespace Opm {
    class DeckKeyword;
    class ParseContext;
    class ErrorGuard;
} // namespace Opm

namespace Opm {

    /// Normalise RPTSHCED keyword specification into sequence of mnemonics
    /// and associate values.
    ///
    /// Simple convenience/helper function only.
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
    RPTKeywordNormalisation::MnemonicMap
    normaliseRptSchedKeyword(const DeckKeyword&  kw,
                             const ParseContext& parseContext,
                             ErrorGuard&         errors);

} // namespace Opm

#endif // OPM_RPTSHCED_KEYWORD_NORMALISATION_HPP_INCLUDED
