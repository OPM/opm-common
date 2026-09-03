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
#include <optional>
#include <string>
#include <vector>

namespace Opm {

class KeywordLocation;

/// Scales finite, non-negative \p fractions to sum to one.
///
/// Accepts sums near one and rejects larger deviations.
///
/// \param what Names the composition in diagnostics.
///
/// \return The original sum when normalization should be reported; otherwise
///         std::nullopt.  Does not log, so callers can report once per table
///         via warnNormalizedMoleFractions().
///
/// \throw OpmInputError for invalid fractions or an out-of-tolerance sum.
std::optional<double> normalizeMoleFractions(std::vector<double>& fractions,
                                             const std::string& what,
                                             const KeywordLocation& location);

/// Warns that one or more compositions were normalized.
/// \param others Number of additional normalized compositions.
void warnNormalizedMoleFractions(const std::string& what,
                                 double sum,
                                 std::size_t others,
                                 const KeywordLocation& location);

} // namespace Opm

#endif // OPM_NORMALIZE_MOLE_FRACTIONS_HPP
