/*
  Copyright 2010, 2019 SINTEF Digital
  Copyright 2010, 2019 Equinor ASA

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

#include <config.h>
#include <opm/common/utility/numeric/RootFinders.hpp>

#include <opm/common/ErrorMacros.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <fmt/format.h>

#include <stdexcept>

namespace Opm
{

double ThrowOnError::handleBracketingFailure(const double x0, const double x1,
                                             const double f0, const double f1)
{
    const auto str = fmt::format("Error in parameters, zero not bracketed: "
                                 "[a, b] = [{}, {}]    f(a) = {}    f(b) = {}",
                                 x0, x1, f0, f1);
    OpmLog::debug(str);
    OPM_THROW_NOLOG(std::runtime_error, str);
    return -1e100; // Never reached.
}

double ThrowOnError::handleTooManyIterations(const double x0,
                                            const double x1, const int maxiter)
{
    OPM_THROW(std::runtime_error,
              fmt::format("Maximum number of iterations exceeded: {}\n"
                          "Current interval is [{}, {}] abs(x0-91) {}",
                          maxiter, std::min(x0, x1), std::max(x0, x1), std::abs(x0-x1)));
    return -1e100; // Never reached.
}

double WarnAndContinueOnError::handleBracketingFailure(const double x0, const double x1,
                                                       const double f0, const double f1)
{
    OPM_REPORT;
    OpmLog::warning(fmt::format("Error in parameters, zero not bracketed: "
                                "[a, b] = [{}, {}]    f(a) = {}   f(b) = {}",
                                x0, x1, f0, f1));
    return std::fabs(f0) < std::fabs(f1) ? x0 : x1;
}

double WarnAndContinueOnError::handleTooManyIterations(const double x0,
                                                       const double x1, const int maxiter)
{
    OPM_REPORT;
    OpmLog::warning(fmt::format("Maximum number of iterations exceeded: {}, "
                                "current interval is [{}, {}]  abs(x0-x1) {}",
                                maxiter, std::abs(x0-x1)));
    return 0.5*(x0 + x1);
}

} // namespace Opm
