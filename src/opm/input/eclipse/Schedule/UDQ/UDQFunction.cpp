/*
  Copyright 2019 Statoil ASA.

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

#include <opm/input/eclipse/Schedule/UDQ/UDQFunction.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Opm {

UDQFunction::UDQFunction(const std::string& name)
    : m_name(name)
    , func_type(UDQ::funcType(name))
{}

UDQFunction::UDQFunction(const std::string& name, UDQTokenType funcType)
    : m_name(name)
    , func_type(funcType)
{}

UDQTokenType UDQFunction::type() const
{
    return this->func_type;
}

const std::string& UDQFunction::name() const
{
    return this->m_name;
}

bool UDQFunction::operator==(const UDQFunction& data) const
{
    return this->name() == data.name() &&
           this->type() == data.type();
}

UDQScalarFunction::UDQScalarFunction(const std::string&name, std::function<UDQSet(const UDQSet& arg)> f)
    : UDQFunction(name)
    , func(std::move(f))
{}

UDQSet UDQScalarFunction::eval(const UDQSet& arg) const
{
    return this->func(arg);
}

UDQSet UDQUnaryElementalFunction::eval(const UDQSet& arg) const
{
    return this->func(arg);
}

//-----------------------------------------------------------------

UDQSet UDQScalarFunction::UDQ_MIN(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("MIN");
    }

    return UDQSet::scalar("MIN", *std::min_element(defined_values.begin(), defined_values.end()));
}

UDQSet UDQScalarFunction::UDQ_MAX(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("MAX");
    }

    return UDQSet::scalar("MAX", *std::max_element(defined_values.begin(), defined_values.end()));
}

UDQSet UDQScalarFunction::SUM(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("SUM");
    }

    return UDQSet::scalar("SUM", std::accumulate(defined_values.begin(), defined_values.end(), 0.0));
}

UDQSet UDQScalarFunction::PROD(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("PROD");
    }

    return UDQSet::scalar("PROD", std::accumulate(defined_values.begin(), defined_values.end(),
                                                  1.0, std::multiplies<double>{}));
}

UDQSet UDQScalarFunction::AVEA(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("AVEA");
    }

    return UDQSet::scalar("AVEA", std::accumulate( defined_values.begin(), defined_values.end(), 0.0)
                          / defined_values.size());
}

UDQSet UDQScalarFunction::AVEG(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("AVEG");
    }

    if (std::find_if(defined_values.begin(), defined_values.end(), [](double x) { return x <= 0; }) != defined_values.end())
    {
        throw std::invalid_argument("Function AVEG must have only positive arguments");
    }

    double log_mean = std::accumulate( defined_values.begin(), defined_values.end(), 0.0,
                                       [](double x, double y) { return x + std::log(y); }) / defined_values.size();

    return UDQSet::scalar("AVEG", std::exp(log_mean));
}

UDQSet UDQScalarFunction::AVEH(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("AVEH");
    }

    return UDQSet::scalar("AVEH", defined_values.size() / std::accumulate(defined_values.begin(), defined_values.end(), 0.0, [](double x, double y) { return x + 1.0/y; }));
}

UDQSet UDQScalarFunction::NORMI(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("NORMI");
    }

    return UDQSet::scalar("NORMI", std::accumulate( defined_values.begin(), defined_values.end(), 0.0,
                                                    [](double x, double y) { return std::max(x, std::fabs(y)); }));
}

UDQSet UDQScalarFunction::NORM1(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("NORM1");
    }

    return UDQSet::scalar("NORM1", std::accumulate( defined_values.begin(), defined_values.end(), 0.0,
                                                    [](double x, double y) { return x + std::fabs(y); }));
}

UDQSet UDQScalarFunction::NORM2(const UDQSet& arg)
{
    auto defined_values = arg.defined_values();
    if (defined_values.empty()) {
        return UDQSet::empty("NORM2");
    }

    return UDQSet::scalar("NORM2", std::sqrt(std::inner_product(defined_values.begin(), defined_values.end(),
                                                                defined_values.begin(), 0.0)));
}

UDQUnaryElementalFunction::UDQUnaryElementalFunction(const std::string&name, std::function<UDQSet(const UDQSet& arg)> f)
    : UDQFunction(name)
    , func(f)
{}

UDQSet UDQUnaryElementalFunction::ABS(const UDQSet& arg)
{
    auto result = arg;
    for (std::size_t index = 0; index < result.size(); ++index) {
        auto& udq_value = result[index];
        if (udq_value) {
            result.assign(index, std::fabs(udq_value.get()));
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::DEF(const UDQSet& arg)
{
    auto result = arg;
    for (std::size_t index=0; index < result.size(); ++index) {
        auto& udq_value = result[index];
        if (udq_value) {
            result.assign(index, 1);
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::UNDEF(const UDQSet& arg)
{
    UDQSet result(arg.name(), arg.size());
    for (std::size_t index=0; index < result.size(); ++index) {
        auto& udq_value = arg[index];
        if (!udq_value) {
            result.assign( index, 1 );
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::IDV(const UDQSet& arg)
{
    auto result = arg;
    for (std::size_t index = 0; index < result.size(); ++index) {
        result.assign(index, arg[index].defined());
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::EXP(const UDQSet& arg)
{
    auto result = arg;
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (const auto& udq_value = result[index]; udq_value) {
            result.assign(index, std::exp(udq_value.get()));
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::NINT(const UDQSet& arg)
{
    auto result = arg;
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (const auto& udq_value = result[index]; udq_value) {
            result.assign(index, std::nearbyint(udq_value.get()));
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::RANDN(std::mt19937& rng, const UDQSet& arg)
{
    auto result = arg;
    std::normal_distribution<double> dist(0.0, 1.0);
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto& udq_value = result[index]; udq_value) {
            result.assign(index, dist(rng));
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::RANDU(std::mt19937& rng, const UDQSet& arg)
{
    auto result = arg;
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto& udq_value = result[index]; udq_value) {
            result.assign(index, dist(rng));
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::LN(const UDQSet& arg)
{
    auto result = arg;
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto& udq_value = result[index]; udq_value) {
            if (const double elm = udq_value.get(); elm > 0.0) {
                result.assign(index, std::log(elm));
            }
            else {
                throw std::invalid_argument {
                    "Argument: " + std::to_string(elm) + " invalid for function LN"
                };
            }
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::LOG(const UDQSet& arg)
{
    auto result = arg;
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto& udq_value = result[index]; udq_value) {
            if (const double elm = udq_value.get(); elm > 0.0) {
                result.assign(index, std::log10(elm));
            }
            else {
                throw std::invalid_argument {
                    "Argument: " + std::to_string(elm) + " invalid for function LOG"
                };
            }
        }
    }

    return result;
}

namespace {

    UDQSet udq_union(const UDQSet& arg1, const UDQSet& arg2)
    {
        if (arg1.size() != arg2.size()) {
            throw std::invalid_argument {
                "UDQ sets have incompatible size"
            };
        }

        UDQSet result = arg1;
        for (std::size_t index = 0; index < result.size(); ++index) {
            const auto& elm1 = arg1[index];
            const auto& elm2 = arg2[index];
            if (elm1.defined() != elm2.defined()) {
                if (elm1) {
                    result.assign(index, elm1.get());
                }

                if (elm2) {
                    result.assign(index, elm2.get());
                }
            }
        }

        return result;
    }

}

UDQSet UDQUnaryElementalFunction::SORT(const UDQSet& arg, bool ascending)
{
    auto sort_nodes = std::vector<std::pair<std::size_t, double>> {};

    if (ascending) {
        for (std::size_t index = 0; index < arg.size(); ++index) {
            if (const auto& value = arg[index]; value.defined()) {
                sort_nodes.emplace_back(index, value.get());
            }
        }
    }
    else {
        for (std::size_t index = 0; index < arg.size(); ++index) {
            if (const auto& value = arg[index]; value.defined()) {
                sort_nodes.emplace_back(index, -value.get());
            }
        }
    }

    std::sort(sort_nodes.begin(), sort_nodes.end(),
              [](const auto& s1, const auto& s2)
              {
                  return s1.second < s2.second;
              });

    auto result = arg;
    double sort_value = 1;
    for (const auto& node : sort_nodes) {
        const auto& index = node.first;
        if (result[index].defined()) {
            result.assign(index, sort_value);
            sort_value += 1;
        }
    }

    return result;
}

UDQSet UDQUnaryElementalFunction::SORTD(const UDQSet& arg)
{
    return UDQUnaryElementalFunction::SORT(arg, false);
}

UDQSet UDQUnaryElementalFunction::SORTA(const UDQSet& arg)
{
    return UDQUnaryElementalFunction::SORT(arg, true);
}

UDQBinaryFunction::UDQBinaryFunction(const std::string& name, std::function<UDQSet(const UDQSet& lhs, const UDQSet& rhs)> f)
    : UDQFunction(name)
    , func(std::move(f))
{}

UDQSet UDQBinaryFunction::eval(const UDQSet& lhs, const UDQSet& rhs) const
{
    return this->func(lhs, rhs);
}

UDQSet UDQBinaryFunction::LE(double eps, const UDQSet& lhs, const UDQSet& rhs)
{
    auto result = lhs - rhs;
    auto rel_diff = result / lhs;

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto elm = result[index]; elm) {
            if (const double abs_diff = elm.get(); abs_diff == 0) {
                result.assign(index, 1);
            }
            else {
                result.assign(index, ! (rel_diff[index].get() > eps));
            }
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::GE(double eps, const UDQSet& lhs, const UDQSet& rhs)
{
    auto result = lhs - rhs;
    auto rel_diff = result / lhs;

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto elm = result[index]; elm) {
            if (const double abs_diff = elm.get(); abs_diff == 0) {
                result.assign(index, 1);
            }
            else {
                result.assign(index, ! (rel_diff[index].get() < -eps));
            }
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::EQ(double eps, const UDQSet& lhs, const UDQSet& rhs)
{
    auto result = lhs - rhs;
    auto rel_diff = result / lhs;

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto elm = result[index]; elm) {
            if (const double abs_diff = elm.get(); abs_diff == 0) {
                result.assign(index, 1);
            }
            else {
                result.assign(index, ! (std::fabs(rel_diff[index].get()) > eps));
            }
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::NE(double eps, const UDQSet& lhs, const UDQSet& rhs)
{
    auto result = UDQBinaryFunction::EQ(eps, lhs, rhs);
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto elm = result[index]; elm) {
            result.assign(index, 1 - elm.get());
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::GT(const UDQSet& lhs, const UDQSet& rhs)
{
    auto result = lhs - rhs;

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto elm = result[index]; elm) {
            result.assign(index, elm.get() > 0.0);
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::LT(const UDQSet& lhs, const UDQSet& rhs)
{
    auto result = lhs - rhs;

    for (std::size_t index = 0; index < result.size(); ++index) {
        if (auto elm = result[index]; elm) {
            result.assign(index, elm.get() < 0.0);
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::ADD(const UDQSet& lhs, const UDQSet& rhs)
{
    return lhs + rhs;
}

UDQSet UDQBinaryFunction::UADD(const UDQSet& lhs, const UDQSet& rhs)
{
    UDQSet result = udq_union(lhs,rhs);
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm) {
            result.assign(index, rhs_elm.get() + lhs_elm.get());
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::UMUL(const UDQSet& lhs, const UDQSet& rhs)
{
    UDQSet result = udq_union(lhs, rhs);
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm) {
            result.assign(index, rhs_elm.get() * lhs_elm.get());
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::UMIN(const UDQSet& lhs, const UDQSet& rhs)
{
    UDQSet result = udq_union(lhs, rhs);
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm) {
            result.assign(index, std::min(rhs_elm.get(), lhs_elm.get()));
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::UMAX(const UDQSet& lhs, const UDQSet& rhs)
{
    UDQSet result = udq_union(lhs, rhs);
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm) {
            result.assign(index, std::max(rhs_elm.get(), lhs_elm.get()));
        }
    }

    return result;
}

UDQSet UDQBinaryFunction::MUL(const UDQSet& lhs, const UDQSet& rhs)
{
    return lhs * rhs;
}

UDQSet UDQBinaryFunction::SUB(const UDQSet& lhs, const UDQSet& rhs)
{
    return lhs - rhs;
}

UDQSet UDQBinaryFunction::DIV(const UDQSet& lhs, const UDQSet& rhs)
{
    return lhs / rhs;
}

UDQSet UDQBinaryFunction::POW(const UDQSet& lhs, const UDQSet& rhs)
{
    UDQSet result = lhs;
    for (std::size_t index = 0; index < result.size(); ++index) {
        auto& lhs_elm = lhs[index];
        auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm) {
            result.assign(index, std::pow(lhs_elm.get(), rhs_elm.get()));
        }
    }

    return result;
}

}
