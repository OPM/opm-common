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
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <random>
#include <numeric>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunction.hpp>

namespace Opm {

UDQFunction::UDQFunction(const std::string& name) :
        m_name(name)
{
}

const std::string& UDQFunction::name() const {
    return this->m_name;
}

UDQScalarFunction::UDQScalarFunction(const std::string&name, std::function<UDQScalar(const UDQSet& arg)> f) :
    UDQFunction(name),
    func(std::move(f))
{
}



UDQScalar UDQScalarFunction::eval(const UDQSet& arg) const {
    return this->func(arg);
}


UDQSet UDQUnaryElementalFunction::eval(const UDQSet& arg) const {
    return this->func(arg);
}

/*****************************************************************/
UDQScalar UDQScalarFunction::MIN(const UDQSet& arg) {
    std::vector<double> defined_values = arg.defined_values();
    if (defined_values.empty())
        return {};

    return *std::min_element(defined_values.begin(), defined_values.end());
}

UDQScalar UDQScalarFunction::MAX(const UDQSet& arg) {
    std::vector<double> defined_values = arg.defined_values();
    if (defined_values.empty())
        return {};

    return *std::max_element(defined_values.begin(), defined_values.end());
}

UDQScalar UDQScalarFunction::SUM(const UDQSet& arg) {
    std::vector<double> defined_values = arg.defined_values();
    return std::accumulate(defined_values.begin(), defined_values.end(), 0.0);
}

UDQScalar UDQScalarFunction::PROD(const UDQSet& arg) {
    std::vector<double> defined_values = arg.defined_values();
    return std::accumulate(defined_values.begin(), defined_values.end(), 1.0, std::multiplies<double>{});
}

UDQScalar UDQScalarFunction::AVEA(const UDQSet& arg) {
    std::vector<double> defined_values = arg.defined_values();
    if (defined_values.empty())
        return {};
    return std::accumulate( defined_values.begin(), defined_values.end(), 0.0) / defined_values.size();
}

UDQScalar UDQScalarFunction::AVEG(const UDQSet& arg) {
    std::vector<double> defined_values = arg.defined_values();
    if (defined_values.empty())
        return {};

    if (std::find_if(defined_values.begin(), defined_values.end(), [](double x) { return x <= 0; }) != defined_values.end())
        throw std::invalid_argument("Function AVEG must have only positive arguments");

    double log_mean = std::accumulate( defined_values.begin(), defined_values.end(), 0.0, [](double x, double y) { return x + std::log(std::fabs(y)); }) / defined_values.size();
    return std::exp( log_mean );
}


UDQScalar UDQScalarFunction::AVEH(const UDQSet& arg) {
    std::vector<double> defined_values = arg.defined_values();
    if (defined_values.empty())
        return {};

    return defined_values.size() / std::accumulate(defined_values.begin(), defined_values.end(), 0.0, [](double x, double y) { return x + 1.0/y; });
}


UDQScalar UDQScalarFunction::NORM1(const UDQSet& arg) {
    auto defined_values = arg.defined_values();
    return std::accumulate( defined_values.begin(), defined_values.end(), 0.0, [](double x, double y) { return x + std::fabs(y);});
}

UDQScalar UDQScalarFunction::NORM2(const UDQSet& arg) {
    auto defined_values = arg.defined_values();
    return std::sqrt(std::inner_product(defined_values.begin(), defined_values.end(), defined_values.begin(), 0.0));
}

UDQScalar UDQScalarFunction::NORMI(const UDQSet& arg) {
    auto defined_values = arg.defined_values();
    return std::accumulate( defined_values.begin(), defined_values.end(), 0.0, [](const double x, const double y) { return std::max(x, std::fabs(y));});
}


UDQUnaryElementalFunction::UDQUnaryElementalFunction(const std::string&name, std::function<UDQSet(const UDQSet& arg)> f) :
    UDQFunction(name),
    func(f)
{}


UDQSet UDQUnaryElementalFunction::ABS(const UDQSet& arg) {
    auto result = arg;
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value)
            result.assign( index, std::fabs(udq_value.value()));
    }
    return result;
}

UDQSet UDQUnaryElementalFunction::DEF(const UDQSet& arg) {
    auto result = arg;
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value)
            result.assign( index, 1 );
    }
    return result;
}


UDQSet UDQUnaryElementalFunction::UNDEF(const UDQSet& arg) {
    UDQSet result(arg.size());
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = arg[index];
        if (!udq_value)
            result.assign( index, 1 );
    }
    return result;
}


UDQSet UDQUnaryElementalFunction::IDV(const UDQSet& arg) {
    auto result = arg;
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value)
            result.assign( index, 1 );
        else
            result.assign(index , 0);
    }
    return result;
}

UDQSet UDQUnaryElementalFunction::EXP(const UDQSet& arg) {
    auto result = arg;
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value)
            result.assign( index, std::exp(udq_value.value()) );
    }
    return result;
}

UDQSet UDQUnaryElementalFunction::NINT(const UDQSet& arg) {
    auto result = arg;
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value)
            result.assign( index, std::nearbyint(udq_value.value()) );
    }
    return result;
}


UDQSet UDQUnaryElementalFunction::RANDN(std::mt19937& rng, const UDQSet& arg) {
    auto result = arg;
    std::normal_distribution<double> dist(0,1);
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value)
            result.assign( index, dist(rng) );
    }
    return result;
}


UDQSet UDQUnaryElementalFunction::RANDU(std::mt19937& rng, const UDQSet& arg) {
    auto result = arg;
    std::uniform_real_distribution<double> dist(-1,1);
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value)
            result.assign( index, dist(rng) );
    }
    return result;
}





UDQSet UDQUnaryElementalFunction::LN(const UDQSet& arg) {
    auto result = arg;
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value) {
            double elm = udq_value.value();
            if (elm > 0)
                result.assign(index, std::log(elm));
            else
                throw std::invalid_argument("Argument: " + std::to_string(elm) + " invalid for function LN");
        }
    }
    return result;
}


UDQSet UDQUnaryElementalFunction::LOG(const UDQSet& arg) {
    auto result = arg;
    for (std::size_t index=0; index < result.size(); index++) {
        auto& udq_value = result[index];
        if (udq_value) {
            double elm = udq_value.value();
            if (elm > 0)
                result.assign(index, std::log10(elm));
            else
                throw std::invalid_argument("Argument: " + std::to_string(elm) + " invalid for function LOG");
        }
    }
    return result;
}

namespace {

    UDQSet udq_union(const UDQSet& arg1, const UDQSet& arg2) {
        if (arg1.size() != arg2.size())
            throw std::invalid_argument("UDQ sets have incoimpatible size");

        UDQSet result(arg1.size());
        for (std::size_t index = 0; index < result.size(); index++) {
            const auto& elm1 = arg1[index];
            const auto& elm2 = arg2[index];
            if (elm1.defined() != elm2.defined()) {
                if (elm1)
                    result.assign(index, elm1.value());

                if (elm2)
                    result.assign(index, elm2.value());
            }
        }
        return result;
    }


    UDQSet udq_sort(const UDQSet& arg, std::size_t defined_size, const std::function<bool(int,int)>& cmp) {
        auto result = arg;
        std::vector<int> index(defined_size);
        std::iota(index.begin(), index.end(), 0);
        std::sort(index.begin(), index.end(), cmp);

        std::size_t output_index = 0;
        for (auto sort_index : index) {
            while (!result[output_index])
                output_index++;

            result.assign(output_index, sort_index);
            output_index++;
        }
        return result;
    }
}


UDQSet UDQUnaryElementalFunction::SORTA(const UDQSet& arg) {
    auto defined_values = arg.defined_values();
    return udq_sort(arg, defined_values.size(), [&defined_values](int a, int b){ return defined_values[a] < defined_values[b]; });
}

UDQSet UDQUnaryElementalFunction::SORTD(const UDQSet& arg) {
    auto defined_values = arg.defined_values();
    return udq_sort(arg, defined_values.size(), [&defined_values](int a, int b){ return defined_values[a] > defined_values[b]; });
}

UDQBinaryFunction::UDQBinaryFunction(const std::string& name, std::function<UDQSet(const UDQSet& lhs, const UDQSet& rhs)> f) :
    UDQFunction(name),
    func(std::move(f))
{
}


UDQSet UDQBinaryFunction::eval(const UDQSet& lhs, const UDQSet& rhs) const {
    return this->func(lhs, rhs);
}

UDQSet UDQBinaryFunction::LE(double eps, const UDQSet& lhs, const UDQSet& rhs) {
    auto result = lhs - rhs;
    auto rel_diff = result / lhs;

    for (std::size_t index=0; index < result.size(); index++) {
        auto elm = result[index];
        if (elm) {
            double diff = rel_diff[index].value();
            if (diff <= eps)
                result.assign(index, 1);
            else
                result.assign(index, 0);
        }
    }
    return result;
}

UDQSet UDQBinaryFunction::GE(double eps, const UDQSet& lhs, const UDQSet& rhs) {
    auto result = lhs - rhs;
    auto rel_diff = result / lhs;

    for (std::size_t index=0; index < result.size(); index++) {
        auto elm = result[index];
        if (elm) {
            double diff = rel_diff[index].value();
            if (diff >= -eps)
                result.assign(index, 1);
            else
                result.assign(index, 0);
        }
    }
    return result;

}


UDQSet UDQBinaryFunction::EQ(double eps, const UDQSet& lhs, const UDQSet& rhs) {
    auto result = lhs - rhs;
    auto rel_diff = result / lhs;

    for (std::size_t index=0; index < result.size(); index++) {
        auto elm = result[index];
        if (elm) {
            double diff = std::fabs(rel_diff[index].value());
            if (diff <= eps)
                result.assign(index, 1);
            else
                result.assign(index, 0);
        }
    }
    return result;

}

UDQSet UDQBinaryFunction::NE(double eps, const UDQSet& lhs, const UDQSet& rhs) {
    auto result = UDQBinaryFunction::EQ(eps, lhs, rhs);
    for (std::size_t index=0; index < result.size(); index++) {
        auto elm = result[index];
        if (elm)
            result.assign(index, 1 - elm.value());
    }
    return result;
}


UDQSet UDQBinaryFunction::GT(const UDQSet& lhs, const UDQSet& rhs) {
    auto result = lhs - rhs;

    for (std::size_t index=0; index < result.size(); index++) {
        auto elm = result[index];
        if (elm) {
            double diff = elm.value();
            if (diff > 0)
                result.assign(index, 1);
            else
                result.assign(index, 0);
        }
    }
    return result;
}

UDQSet UDQBinaryFunction::LT(const UDQSet& lhs, const UDQSet& rhs) {
    auto result = lhs - rhs;

    for (std::size_t index=0; index < result.size(); index++) {
        auto elm = result[index];
        if (elm) {
            double diff = elm.value();
            if (diff < 0)
                result.assign(index, 1);
            else
                result.assign(index, 0);
        }
    }
    return result;
}

UDQSet UDQBinaryFunction::ADD(const UDQSet& lhs, const UDQSet& rhs) {
    return lhs + rhs;
}



UDQSet UDQBinaryFunction::UADD(const UDQSet& lhs, const UDQSet& rhs) {
    UDQSet result = udq_union(lhs,rhs);
    for (std::size_t index=0; index < lhs.size(); index++) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm)
            result.assign(index, rhs_elm.value() + lhs_elm.value());
    }
    return result;
}

UDQSet UDQBinaryFunction::UMUL(const UDQSet& lhs, const UDQSet& rhs) {
    UDQSet result = udq_union(lhs, rhs);
    for (std::size_t index=0; index < lhs.size(); index++) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm)
            result.assign(index, rhs_elm.value() * lhs_elm.value());
    }
    return result;
}

UDQSet UDQBinaryFunction::UMIN(const UDQSet& lhs, const UDQSet& rhs) {
    UDQSet result = udq_union(lhs, rhs);
    for (std::size_t index=0; index < lhs.size(); index++) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm)
            result.assign(index, std::min(rhs_elm.value(), lhs_elm.value()));
    }
    return result;
}


UDQSet UDQBinaryFunction::UMAX(const UDQSet& lhs, const UDQSet& rhs) {
    UDQSet result = udq_union(lhs, rhs);
    for (std::size_t index=0; index < lhs.size(); index++) {
        const auto& lhs_elm = lhs[index];
        const auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm)
            result.assign(index, std::max(rhs_elm.value(), lhs_elm.value()));
    }
    return result;
}





UDQSet UDQBinaryFunction::MUL(const UDQSet& lhs, const UDQSet& rhs) {
    return lhs * rhs;
}

UDQSet UDQBinaryFunction::SUB(const UDQSet& lhs, const UDQSet& rhs) {
    return lhs - rhs;
}

UDQSet UDQBinaryFunction::DIV(const UDQSet& lhs, const UDQSet& rhs) {
    return lhs / rhs;
}

UDQSet UDQBinaryFunction::POW(const UDQSet& lhs, const UDQSet& rhs) {
    UDQSet result(lhs.size());
    for (std::size_t index = 0; index < result.size(); index++) {
        auto& lhs_elm = lhs[index];
        auto& rhs_elm = rhs[index];

        if (lhs_elm && rhs_elm)
            result.assign(index, std::pow(lhs_elm.value(), rhs_elm.value()));
    }
    return result;
}

}
