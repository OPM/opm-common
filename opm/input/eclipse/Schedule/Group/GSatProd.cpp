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

#include <stdexcept>

namespace Opm {

GSatProd GSatProd::serializationTestObject()
{
    GSatProd result;
    result.groups_ = { {"test1", {1.0,2.0,3.0,4.0,5.0} } };

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

void GSatProd::assign(const std::string& name,
                      const double       oil_rate,
                      const double       gas_rate,
                      const double       water_rate,
                      const double       resv_rate,
                      const double       glift_rate)
{
    using Rate = GSatProdGroup::Rate;

    auto& rate = this->groups_[name].rate;

    rate[Rate::Oil] = oil_rate;
    rate[Rate::Gas] = gas_rate;
    rate[Rate::Water] = water_rate;
    rate[Rate::Resv] = resv_rate;
    rate[Rate::GLift] = glift_rate;
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
