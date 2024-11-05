/*
  Copyright 2018 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Action/ActionContext.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include <fmt/format.h>

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {
    std::string combinedKey(std::string_view function,
                            std::string_view argument)
    {
        return fmt::format("{}:{}", function, argument);
    }
}

Opm::Action::Context::Context(const SummaryState& summary_state,
                              const WListManager& wlm)
    : summaryState_ { std::cref(summary_state) }
    , wListMgr_     { std::cref(wlm) }
{
    for (const auto& [month, idx] : TimeService::eclipseMonthIndices()) {
        this->add(month, idx);
    }
}

void Opm::Action::Context::add(std::string_view func,
                               std::string_view arg,
                               const double     value)
{
    this->add(combinedKey(func, arg), value);
}

void Opm::Action::Context::add(const std::string& func,
                               const double       value)
{
    this->values_.insert_or_assign(func, value);
}

double Opm::Action::Context::get(std::string_view func,
                                 std::string_view arg) const
{
    return this->get(combinedKey(func, arg));
}

double Opm::Action::Context::get(const std::string& key) const
{
    auto iter = this->values_.find(key);

    return (iter == this->values_.end())
        ? this->summaryState_.get().get(key)
        : iter->second;
}

std::vector<std::string>
Opm::Action::Context::wells(const std::string& key) const
{
    return this->summaryState_.get().wells(key);
}
