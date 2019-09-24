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

#ifndef GUIDE_RATE_CONFIG_HPP
#define GUIDE_RATE_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <memory>

#include <opm/parser/eclipse/EclipseState/Schedule/Group/GuideRateModel.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group2.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>

namespace Opm {


class GuideRateConfig {
public:

struct Well {
    double guide_rate;
    Well2::GuideRateTarget target;
    double scaling_factor;
};

struct Group {
    double guide_rate;
    Group2::GuideRateTarget target;
};

    const GuideRateModel& model() const;
    bool has_model() const;
    bool update_model(const GuideRateModel& model);
    void update_well(const Well2& well);
    void update_group(const Group2& group);
    const Well& well(const std::string& well) const;
    const Group& group(const std::string& group) const;
    bool has_well(const std::string& well) const;
    bool has_group(const std::string& group) const;
private:
    std::shared_ptr<GuideRateModel> m_model;
    std::unordered_map<std::string, Well> wells;
    std::unordered_map<std::string, Group> groups;
};

}

#endif
