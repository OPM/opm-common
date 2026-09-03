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

#include <opm/input/eclipse/EclipseState/Compositional/NormalizeMoleFractions.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace {

// Accept compositions rounded to five or six decimal places.
constexpr double moleFractionTolerance = 1.0e-4;

double exactSumSlack(const std::size_t numValues)
{
    // One rounding per value, and one more per addition.
    return 2.0 * numValues * std::numeric_limits<double>::epsilon();
}

} // Anonymous namespace

namespace Opm {

std::optional<double> normalizeMoleFractions(std::vector<double>& fractions,
                                             const std::string& what,
                                             const KeywordLocation& location)
{
    for (std::size_t component = 0; component < fractions.size(); ++component) {
        const double fraction = fractions[component];
        if (!std::isfinite(fraction) || fraction < 0.0) {
            throw OpmInputError(fmt::format("Mole fraction {} of {} is {}, "
                                            "but mole fractions must be finite and non-negative.",
                                            component + 1, what, fraction),
                                location);
        }
    }

    const double sum = std::accumulate(fractions.begin(), fractions.end(), 0.0);

    // Finite values may overflow when summed.
    if (!std::isfinite(sum)) {
        throw OpmInputError(fmt::format("The mole fractions of {} sum to {}, "
                                        "which is not a finite number.", what, sum),
                            location);
    }

    const double deviation = std::abs(sum - 1.0);

    if (deviation > moleFractionTolerance) {
        throw OpmInputError(fmt::format("The mole fractions of {} sum to {}, "
                                        "which is not one.", what, sum),
                            location);
    }

    for (auto& x : fractions) {
        x /= sum;
    }

    // Do not warn about floating-point summation error.
    if (deviation <= exactSumSlack(fractions.size())) {
        return {};
    }

    return { sum };
}

void warnNormalizedMoleFractions(const std::string& what,
                                 const double sum,
                                 const std::size_t others,
                                 const KeywordLocation& location)
{
    auto message = fmt::format("The mole fractions of {} sum to {}: they should "
                               "sum to unity and have been normalized", what, sum);

    if (others > 0) {
        message += fmt::format(", as {} {} further composition{} in this keyword",
                               (others == 1) ? "was" : "were",
                               others,
                               (others == 1) ? "" : "s");
    }

    message += '.';

    // Format the fixed template separately because 'message' contains deck input.
    OpmLog::warning(message +
                    OpmInputError::format("\nIn {keyword} in {file} line {line}.", location));
}

} // namespace Opm
