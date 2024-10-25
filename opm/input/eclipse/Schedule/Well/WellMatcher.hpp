/*
  Copyright 2019 Equinor ASA.

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
#ifndef WELL_MATCHER_HPP
#define WELL_MATCHER_HPP

#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>
#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>

#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

class WellMatcher
{
public:
    WellMatcher() = default;
    explicit WellMatcher(NameOrder&& well_order);
    explicit WellMatcher(const NameOrder* well_order);
    explicit WellMatcher(std::initializer_list<std::string> wells);
    explicit WellMatcher(const std::vector<std::string>& wells);
    WellMatcher(const NameOrder* well_order, const WListManager& wlm);

    std::vector<std::string> sort(std::vector<std::string> wells) const;
    std::vector<std::string> wells(const std::string& pattern) const;

    const std::vector<std::string>& wells() const;

private:
    std::unique_ptr<NameOrder> m_wo{};
    const NameOrder* m_well_order{nullptr};
    std::optional<std::reference_wrapper<const WListManager>> m_wlm{};
};

} // namespace Opm

#endif // WELL_MATCHER_HPP
