/*
  Copyright 2024 Equinor ASA.

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

#include <opm/input/eclipse/Deck/UDAValue.hpp>

#include <opm/input/eclipse/Schedule/Group/GSatProd.hpp>
#include "../eval_uda.hpp"

#include <stdexcept>

namespace Opm {

GSatProd GSatProd::serializationTestObject()
{
    GSatProd result;
    result.groups_ = { {"test1", {UDAValue(1.0), UDAValue(2.0), UDAValue(3.0), 4.0,
                                  5.0, 6.0, UnitSystem::serializationTestObject()} } };

    return result;
}

bool GSatProd::has(const std::string& name) const
{
    return this->groups_.find(name) != this->groups_.end();
}

const GSatProd::GSatProdGroup& GSatProd::get(const std::string& name) const
{
    auto it = this->groups_.find(name);
    if (it == this->groups_.end()) {
        throw std::invalid_argument {
            "Current GSatPRod obj. does not contain '" + name + "'."
        };
    }

    return it->second;
}

const GSatProd::GSatProdGroupProp GSatProd::get(const std::string& name, const SummaryState& st) const
{
    using Rate = GSatProdGroupProp::Rate;

    GSatProdGroupProp prop;
    const GSatProd::GSatProdGroup& group = this->get(name);
    prop.rate[Rate::Oil] = UDA::eval_group_uda(group.oil_rate, name, st, group.udq_undefined);
    prop.rate[Rate::Gas] = UDA::eval_group_uda(group.gas_rate, name, st, group.udq_undefined);
    prop.rate[Rate::Water] = UDA::eval_group_uda(group.water_rate, name, st, group.udq_undefined);
    
    return prop;
}

void GSatProd::assign(const std::string& name,
                      const UDAValue&    oil_rate,
                      const UDAValue&    gas_rate,
                      const UDAValue&    water_rate,
                      const double       resv_rate,
                      const double       glift_rate,
                      double             udq_undefined,
                      const UnitSystem&  unit_system)
{
    GSatProd::GSatProdGroup& group = groups_[name];

    group.oil_rate = oil_rate;
    group.gas_rate = gas_rate;
    group.water_rate = water_rate;
    group.resv_rate = resv_rate;
    group.glift_rate = glift_rate;
    group.udq_undefined = udq_undefined;
    group.unit_system = unit_system;
}

std::size_t GSatProd::size() const
{
    return this->groups_.size();
}

bool GSatProd::operator==(const GSatProd& data) const
{
    return this->groups_ == data.groups_;
}

} // namespace Opm
