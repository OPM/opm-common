/*
  Copyright 2026 SINTEF Digital

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

#ifndef OPM_NORMALIZE_MOLE_FRACTIONS_HPP
#define OPM_NORMALIZE_MOLE_FRACTIONS_HPP

#include <cstddef>
#include <string>
#include <vector>

namespace Opm {

class KeywordLocation;

/// How far a set of mole fractions may sum away from one before it is
/// rejected.  Writing a composition with a few digits costs this much.
double moleFractionTolerance();

/// Slack below which a sum counts as exactly one: summing n values of order
/// one costs about n roundings, and representing them costs as many again.
double exactSumSlack(std::size_t numValues);

/// Scales \p fractions so that they sum to one, and says so when the scaling
/// was more than the arithmetic of the sum.
///
/// \param what  Names the input in the warning, e.g. "row 2 of COMPVD table 1"
///              or "stream 'ISTR'".
///
/// \throw OpmInputError when a fraction is non-finite, or when the sum is too
///        far from one to be the rounding of the values.
void normalizeMoleFractions(std::vector<double>& fractions,
                            const std::string& what,
                            const KeywordLocation& location);

} // namespace Opm

#endif // OPM_NORMALIZE_MOLE_FRACTIONS_HPP
