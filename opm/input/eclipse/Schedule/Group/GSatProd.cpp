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

#include <opm/input/eclipse/Schedule/Group/GSatProd.hpp>

#include <opm/input/eclipse/Deck/UDAValue.hpp>
#include "../eval_uda.hpp"

#include <stdexcept>

namespace Opm {

GSatProd GSatProd::serializationTestObject()
{
    GSatProd result;
    result.groups_ = { {"test1", {{UDAValue(1.0), UDAValue(2.0), UDAValue(3.0),
                                   UDAValue(4.0), UDAValue(5.0)}, 6.0} } };

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
    prop.rate[Rate::Oil] = UDA::eval_group_uda(group.rate[Rate::Oil], name, st, group.udq_undefined);
    prop.rate[Rate::Gas] = UDA::eval_group_uda(group.rate[Rate::Gas], name, st, group.udq_undefined);
    prop.rate[Rate::Water] = UDA::eval_group_uda(group.rate[Rate::Water], name, st, group.udq_undefined);
    prop.rate[Rate::Resv] = UDA::eval_group_uda(group.rate[Rate::Resv], name, st, group.udq_undefined);
    prop.rate[Rate::GLift] = UDA::eval_group_uda(group.rate[Rate::GLift], name, st, group.udq_undefined);

    return prop;
}

void GSatProd::assign(const std::string& name,
                      const UDAValue&    oil_rate,
                      const UDAValue&    gas_rate,
                      const UDAValue&    water_rate,
                      const UDAValue&    resv_rate,
                      const UDAValue&    glift_rate,
                      double             udq_undefined)
{
    using Rate = GSatProdGroupProp::Rate;

    GSatProd::GSatProdGroup& group = groups_[name];

    group.rate[Rate::Oil] = oil_rate;
    group.rate[Rate::Gas] = gas_rate;
    group.rate[Rate::Water] = water_rate;
    group.rate[Rate::Resv] = resv_rate;
    group.rate[Rate::GLift] = glift_rate;
    group.udq_undefined = udq_undefined;
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
