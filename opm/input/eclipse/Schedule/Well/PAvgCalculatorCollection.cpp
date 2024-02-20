/*
  Copyright 2020 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Well/PAvgCalculatorCollection.hpp>

#include <opm/input/eclipse/Schedule/Well/PAvgCalculator.hpp>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace Opm {

std::size_t
PAvgCalculatorCollection::setCalculator(const std::size_t wellID,
                                        CalculatorPtr     calculator)
{
    if (auto indexPos = this->index_.find(wellID);
        indexPos != this->index_.end())
    {
        this->calculators_[indexPos->second] = std::move(calculator);
        return indexPos->second;
    }
    else {
        const auto ix = this->calculators_.size();
        this->index_.insert_or_assign(wellID, ix);

        this->calculators_.push_back(std::move(calculator));

        return ix;
    }
}

void PAvgCalculatorCollection::pruneInactiveWBPCells(ActivePredicate isActive)
{
    auto wbpCells = std::vector<std::size_t>{};
    auto calcCellSize = std::vector<std::vector<bool>::size_type>{};
    calcCellSize.reserve(this->calculators_.size());

    for (const auto& calculatorPtr : this->calculators_) {
        const auto& calcWBPCells = calculatorPtr->allWBPCells();
        wbpCells.insert(wbpCells.end(), calcWBPCells.begin(), calcWBPCells.end());

        calcCellSize.push_back(calcWBPCells.size());
    }

    const auto cellIsActive = isActive(wbpCells);

    auto calcIndex = 0 * this->calculators_.size();
    auto end       = cellIsActive.begin();
    for (auto& calculatorPtr : this->calculators_) {
        auto begin = end;
        end += calcCellSize[calcIndex++];

        calculatorPtr->pruneInactiveWBPCells({ begin, end });
    }
}

PAvgCalculator<double>&
PAvgCalculatorCollection::operator[](const std::size_t i)
{
    return *this->calculators_[i];
}

const PAvgCalculator<double>&
PAvgCalculatorCollection::operator[](const std::size_t i) const
{
    return *this->calculators_[i];
}

bool PAvgCalculatorCollection::empty() const
{
    return this->calculators_.empty();
}

std::size_t PAvgCalculatorCollection::numCalculators() const
{
    return this->calculators_.size();
}

std::vector<std::size_t> PAvgCalculatorCollection::allWBPCells() const
{
    auto wbpCells = std::vector<std::size_t>{};

    for (const auto& calculatorPtr : this->calculators_) {
        const auto& calcWBPCells = calculatorPtr->allWBPCells();
        wbpCells.insert(wbpCells.end(), calcWBPCells.begin(), calcWBPCells.end());
    }

    std::sort(wbpCells.begin(), wbpCells.end());

    return { wbpCells.begin(), std::unique(wbpCells.begin(), wbpCells.end()) };
}

} // namespace Opm
