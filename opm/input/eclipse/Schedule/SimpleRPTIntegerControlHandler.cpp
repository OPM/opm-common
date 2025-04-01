/*
  Copyright 2025 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/SimpleRPTIntegerControlHandler.hpp>

#include <opm/input/eclipse/Schedule/RPTKeywordNormalisation.hpp>

#include <algorithm>
#include <string>
#include <vector>

Opm::RPTKeywordNormalisation::MnemonicMap
Opm::SimpleRPTIntegerControlHandler::
operator()(const std::vector<int>& controlValues) const
{
    auto mnemonics = RPTKeywordNormalisation::MnemonicMap{};

    const auto numValues =
        std::min(controlValues.size(), this->keywords_.size());

    for (auto i = 0*numValues; i < numValues; ++i) {
        mnemonics.emplace_back(this->keywords_[i], controlValues[i]);
    }

    return mnemonics;
}
