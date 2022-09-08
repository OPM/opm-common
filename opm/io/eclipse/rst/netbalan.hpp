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

#ifndef RST_NETBALAN_HPP
#define RST_NETBALAN_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace Opm {
    class UnitSystem;
} // namespace Opm

namespace Opm { namespace RestartIO {

    class RstNetbalan
    {
    public:
        explicit RstNetbalan(const std::vector<int>&    intehead,
                             const std::vector<double>& doubhead,
                             const UnitSystem&          usys);

        double interval() const
        {
            return this->calc_interval_;
        }

        double pressureTolerance() const
        {
            return this->ptol_;
        }

        std::size_t pressureMaxIter() const
        {
            return this->pressure_max_iter_;
        }

        double thpTolerance() const
        {
            return this->thp_tolerance_;
        }

        std::size_t thpMaxIter() const
        {
            return this->thp_max_iter_;
        }

        const std::optional<double>& targetBalanceError() const
        {
            return this->target_branch_balance_error_;
        }

        const std::optional<double>& maxBalanceError() const
        {
            return this->max_branch_balance_error_;
        }

        const std::optional<double>& minTstep() const
        {
            return this->min_tstep_;
        }

    private:
        double calc_interval_;
        double ptol_;
        std::size_t pressure_max_iter_;

        double thp_tolerance_;
        std::size_t thp_max_iter_;

        std::optional<double> target_branch_balance_error_{};
        std::optional<double> max_branch_balance_error_{};
        std::optional<double> min_tstep_{};
    };

}} // namespace Opm::RestartIO

#endif // RST_NETBALAN_HPP
