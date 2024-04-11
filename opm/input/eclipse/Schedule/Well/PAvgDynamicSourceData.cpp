/*
  Copyright 2023 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Well/PAvgDynamicSourceData.hpp>

#include <opm/common/ErrorMacros.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

template<class Scalar>
Opm::PAvgDynamicSourceData<Scalar>::
PAvgDynamicSourceData(const std::vector<std::size_t>& sourceLocations)
    : src_(numSpanItems() * sourceLocations.size(), 0.0)
{
    this->buildLocationMapping(sourceLocations);
}

template<class Scalar>
typename Opm::PAvgDynamicSourceData<Scalar>::template SourceDataSpan<Scalar>
Opm::PAvgDynamicSourceData<Scalar>::operator[](const std::size_t source)
{
    const auto i = this->index(source);
    if (! i.has_value()) {
        OPM_THROW_NOLOG(std::invalid_argument,
                        fmt::format("Dynamic source location "
                                    "'{}' is not registered", source));
    }

    return SourceDataSpan<Scalar>{ &this->src_[*i] };
}

template<class Scalar>
typename Opm::PAvgDynamicSourceData<Scalar>::template SourceDataSpan<const Scalar>
Opm::PAvgDynamicSourceData<Scalar>::operator[](const std::size_t source) const
{
    const auto i = this->index(source);
    if (! i.has_value()) {
        OPM_THROW_NOLOG(std::invalid_argument,
                        fmt::format("Dynamic source location "
                                    "'{}' is not registered", source));
    }

    return SourceDataSpan<const Scalar>{ &this->src_[*i] };
}

template<class Scalar>
typename Opm::PAvgDynamicSourceData<Scalar>::template SourceDataSpan<Scalar>
Opm::PAvgDynamicSourceData<Scalar>::sourceTerm(const std::size_t ix,
                                               std::vector<Scalar>& src)
{
    return SourceDataSpan<Scalar> { &src[ix*numSpanItems() + 0] };
}

template<class Scalar>
void Opm::PAvgDynamicSourceData<Scalar>::
reconstruct(const std::vector<std::size_t>& sourceLocations)
{
    this->src_.assign(numSpanItems() * sourceLocations.size(), 0.0);

    this->buildLocationMapping(sourceLocations);
}

template<class Scalar>
void Opm::PAvgDynamicSourceData<Scalar>::
buildLocationMapping(const std::vector<std::size_t>& sourceLocations)
{
    this->ix_.clear();

    auto ix = typename std::vector<Scalar>::size_type{0};
    for (const auto& srcLoc : sourceLocations) {
        auto elm = this->ix_.emplace(srcLoc, ix++);
        if (! elm.second) {
            OPM_THROW_NOLOG(std::invalid_argument,
                            fmt::format("Failed to set up internal mapping table, "
                                        "single location {} entered multiple times.",
                                        srcLoc));
        }
    }
}

template<class Scalar>
std::optional<typename std::vector<Scalar>::size_type>
Opm::PAvgDynamicSourceData<Scalar>::
index(const std::size_t source) const
{
    auto pos = this->ix_.find(source);
    if (pos == this->ix_.end()) {
        return {};
    }

    return numSpanItems() * this->storageIndex(pos->second);
}

template class Opm::PAvgDynamicSourceData<double>;
template class Opm::PAvgDynamicSourceData<float>;
