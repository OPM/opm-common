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
#include <limits>
#include <numeric>

#include <fmt/format.h>

namespace Opm {

double moleFractionTolerance()
{
    return 1.0e-4;
}

double exactSumSlack(const std::size_t numValues)
{
    return 2.0 * numValues * std::numeric_limits<double>::epsilon();
}

void normalizeMoleFractions(std::vector<double>& fractions,
                            const std::string& what,
                            const KeywordLocation& location)
{
    const double sum = std::accumulate(fractions.begin(), fractions.end(), 0.0);

    // A non-finite fraction makes the sum non-finite, and every comparison
    // against a NaN is false: the checks below would all pass and the
    // fractions would then be "normalized" by dividing through the NaN.
    if (!std::isfinite(sum)) {
        throw OpmInputError(fmt::format("The mole fractions of {} sum to {}, "
                                        "which is not a finite number.", what, sum),
                            location);
    }

    const double deviation = std::abs(sum - 1.0);

    if (deviation > moleFractionTolerance()) {
        throw OpmInputError(fmt::format("The mole fractions of {} sum to {}, "
                                        "which is not one.", what, sum),
                            location);
    }

    if (deviation > exactSumSlack(fractions.size())) {
        // Printed round-trip: a deviation small enough to round away at a
        // fixed precision is exactly the one worth naming.
        OpmLog::warning(fmt::format("The mole fractions of {} sum to {}: they should "
                                    "sum to unity and have been normalized.", what, sum));
    }

    for (auto& x : fractions) {
        x /= sum;
    }
}

} // namespace Opm
