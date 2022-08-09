/*
  Copyright 2022 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along
  with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/io/eclipse/rst/netbalan.hpp>

#include <opm/io/eclipse/RestartFileView.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/doubhead.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace {
    std::size_t maxBalanceIter(const std::vector<int>& intehead)
    {
        return intehead[VI::intehead::NetbalMaxBalanceIter];
    }

    std::size_t maxTHPIter(const std::vector<int>& intehead)
    {
        return intehead[VI::intehead::NetbalMaxTHPIter];
    }

    double calcInterval(const std::vector<double>& doubhead,
                        const Opm::UnitSystem&     usys)
    {
        return usys.to_si(Opm::UnitSystem::measure::time,
                          doubhead[VI::doubhead::Netbalint]);
    }

    double pressureToleranceValue(const std::vector<double>& doubhead,
                                  const Opm::UnitSystem&     usys)
    {
        return usys.to_si(Opm::UnitSystem::measure::pressure,
                          doubhead[VI::doubhead::Netbalnpre]);
    }

    double thpToleranceValue(const std::vector<double>& doubhead,
                             const Opm::UnitSystem&     usys)
    {
        return usys.to_si(Opm::UnitSystem::measure::pressure,
                          doubhead[VI::doubhead::Netbalthpc]);
    }

    bool is_finite_float(const double x)
    {
        return std::abs(static_cast<float>(x)) < 1.0e+20f;
    }

    std::optional<double>
    targetBranchBalanceError(const std::vector<double>& doubhead,
                             const Opm::UnitSystem&     usys)
    {
        const auto trgBE = doubhead[VI::doubhead::Netbaltarerr];

        return is_finite_float(trgBE)
            ? std::optional<double>{ usys.to_si(Opm::UnitSystem::measure::pressure, trgBE) }
            : std::nullopt;
    }

    std::optional<double>
    maxBranchBalanceError(const std::vector<double>& doubhead,
                          const Opm::UnitSystem&     usys)
    {
        const auto maxBE = doubhead[VI::doubhead::Netbalmaxerr];

        return is_finite_float(maxBE)
            ? std::optional<double>{ usys.to_si(Opm::UnitSystem::measure::pressure, maxBE) }
            : std::nullopt;
    }

    std::optional<double>
    minimumTimestepSize(const std::vector<double>& doubhead,
                        const Opm::UnitSystem&     usys)
    {
        const auto minTStep = doubhead[VI::doubhead::Netbalstepsz];

        return (minTStep != VI::DoubHeadValue::NetBalMinTSDefault)
            ? std::optional<double>{ usys.to_si(Opm::UnitSystem::measure::time, minTStep) }
            : std::nullopt;
    }
}

Opm::RestartIO::RstNetbalan::RstNetbalan(const std::vector<int>&    intehead,
                                         const std::vector<double>& doubhead,
                                         const UnitSystem&          usys)
    : calc_interval_              (calcInterval(doubhead, usys))
    , ptol_                       (pressureToleranceValue(doubhead, usys))
    , pressure_max_iter_          (maxBalanceIter(intehead))
    , thp_tolerance_              (thpToleranceValue(doubhead, usys))
    , thp_max_iter_               (maxTHPIter(intehead))
    , target_branch_balance_error_(targetBranchBalanceError(doubhead, usys))
    , max_branch_balance_error_   (maxBranchBalanceError(doubhead, usys))
    , min_tstep_                  (minimumTimestepSize(doubhead, usys))
{}
