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

#include <opm/input/eclipse/Schedule/SummaryState.hpp>

#include <opm/input/eclipse/Deck/UDAValue.hpp>

#include "../eval_uda.hpp"

#include <string>
#include <type_traits>

namespace {
    template <typename Body>
    void rateLoop(Body&& body)
    {
        constexpr auto num_rates =
            static_cast<std::underlying_type_t<Opm::GSatProd::Rate>>
            (Opm::GSatProd::Rate::Num);

        for (auto rate = 0*num_rates; rate < num_rates; ++rate) {
            body(static_cast<Opm::GSatProd::Rate>(rate));
        }
    }
} // Anonymous namespace

Opm::GSatProd::GSatProd(const std::string& group)
    : group_ { group }
{}

Opm::GSatProd
Opm::GSatProd::serializationTestObject()
{
    using namespace std::string_literals;

    auto test_object = GSatProd { "test1"s };

    rateLoop([rate_value = 1.0, &test_object](const Rate r) mutable
    {
        test_object.rate_[r] = UDAValue { rate_value++ };
    });

    return test_object;
}

void Opm::GSatProd::assign(const Values<UDAValue>& input)
{
    this->rate_ = input;
}

std::optional<std::string>
Opm::GSatProd::udq(const Rate r) const
{
    if (const auto& uda = this->rate_[r]; uda.is_numeric()) {
        return {};
    }
    else {
        return { uda.get<std::string>() };
    }
}

double
Opm::GSatProd::getRate(const Rate r, const SummaryState& st) const
{
    return UDA::eval_group_uda(this->rate_[r],
                               this->group_,
                               st, st.get_udq_undefined());
}

Opm::GSatProd::Values<double>
Opm::GSatProd::getRates(const SummaryState& st) const
{
    auto rate_value = Values<double>{};

    rateLoop([this, &st, &rate_value](const Rate r)
    {
        rate_value[r] = this->getRate(r, st);
    });

    return rate_value;
}

bool Opm::GSatProd::operator==(const GSatProd& that) const
{
    return (this->group_ == that.group_)
        && (this->rate_ == that.rate_)
        ;
}
