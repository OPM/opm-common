/*
  Copyright 2016 SINTEF ICT, Applied Mathematics.
  Copyright 2016 Statoil ASA.
  Copyright 2022 Equinor ASA

  This file is part of the Open Porous Media Project (OPM).

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

#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <opm/output/data/InterRegFlowMap.hpp>

#include <opm/output/data/InterRegFlow.hpp>

#include <opm/common/utility/CSRGraphFromCoordinates.hpp>

#include <algorithm>
#include <cassert>
#include <exception>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void
Opm::data::InterRegFlowMap::
addConnection(const int        r1,
              const int        r2,
              const FlowRates& rates)
{
    if ((r1 < 0) || (r2 < 0)) {
        throw std::invalid_argument {
            "Region indices must be non-negative.  Got (r1,r2) = ("
            + std::to_string(r1) + ", " + std::to_string(r2)
            + ')'
        };
    }

    if (r1 == r2) {
        // Internal to a region.  Skip.
        return;
    }

    const auto one   = Window::ElmT{1};
    const auto sign  = (r1 < r2) ? one : -one;
    const auto start = this->rates_.size();

    auto low = r1, high = r2;
    if (std::signbit(sign)) {
        std::swap(low, high);
    }

    this->connections_.addConnection(low, high);

    this->rates_.insert(this->rates_.end(), Window::bufferSize(), Window::ElmT{0});
    Window { this->rates_.begin() + start, this->rates_.end() }.addFlow(sign, rates);
}

void Opm::data::InterRegFlowMap::compress(const std::size_t numRegions)
{
    this->connections_.compress(numRegions);

    const auto v = this->rates_;
    constexpr auto sz = Window::bufferSize();
    const auto& dstIx = this->connections_.compressedIndexMap();

    if (v.size() != dstIx.size()*sz) {
        throw std::logic_error {
            "Flow rates must be provided for each connection"
        };
    }

    auto dst = [this](const Offset start) -> Window
    {
        auto begin = this->rates_.begin() + start*sz;

        return Window { begin, begin + sz };
    };

    auto src = [&v](const Offset start) -> ReadOnlyWindow
    {
        auto begin = v.begin() + start*sz;

        return ReadOnlyWindow { begin, begin + sz };
    };

    this->rates_.assign(this->connections_.columnIndices().size() * sz, Window::ElmT{0});

    const auto numRates = dstIx.size();
    for (auto rateID = 0*numRates; rateID < numRates; ++rateID) {
        dst(dstIx[rateID]) += src(rateID);
    }
}

Opm::data::InterRegFlowMap::Offset
Opm::data::InterRegFlowMap::numRegions() const
{
    return this->connections_.numVertices();
}

std::optional<std::pair<
    Opm::data::InterRegFlowMap::ReadOnlyWindow,
    Opm::data::InterRegFlowMap::ReadOnlyWindow::ElmT
>>
Opm::data::InterRegFlowMap::getInterRegFlows(const int r1, const int r2) const
{
    if ((r1 < 0) || (r2 < 0)) {
        throw std::invalid_argument {
            "Region indices must be non-negative.  Got (r1,r2) = ("
            + std::to_string(r1) + ", " + std::to_string(r2)
            + ')'
        };
    }

    if (r1 == r2) {
        // Internal to a region.  Skip.
        throw std::invalid_argument {
            "Region indices must be distinct.  Got (r1,r2) = ("
            + std::to_string(r1) + ", " + std::to_string(r2)
            + ')'
        };
    }

    using ElmT = ReadOnlyWindow::ElmT;

    const auto one  = ElmT{1};
    const auto sign = (r1 < r2) ? one : -one;

    auto low = r1, high = r2;
    if (std::signbit(sign)) {
        std::swap(low, high);
    }

    const auto& ia = this->connections_.startPointers();
    const auto& ja = this->connections_.columnIndices();

    auto begin = ja.begin() + ia[low + 0];
    auto end   = ja.begin() + ia[low + 1];
    auto pos   = std::lower_bound(begin, end, high);
    if ((pos == end) || (*pos > high)) {
        // High is not connected to low.
        return std::nullopt;
    }

    const auto sz = ReadOnlyWindow::bufferSize();
    const auto windowID = pos - ja.begin();

    auto rateStart = this->rates_.begin() + windowID*sz;

    return { std::pair { ReadOnlyWindow { rateStart, rateStart + sz }, sign } };
}

void Opm::data::InterRegFlowMap::clear()
{
    this->connections_.clear();
    this->rates_.clear();
}
