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

#ifndef OPM_SIMPLE_RPT_INTEGER_CONTROL_HANDLER_HPP_INCLUDED
#define OPM_SIMPLE_RPT_INTEGER_CONTROL_HANDLER_HPP_INCLUDED

#include <opm/input/eclipse/Schedule/RPTKeywordNormalisation.hpp>

#include <initializer_list>
#include <string>
#include <vector>

namespace Opm {

class SimpleRPTIntegerControlHandler
{
public:
    explicit SimpleRPTIntegerControlHandler(std::initializer_list<const char*> keywords)
        : keywords_ { keywords.begin(), keywords.end() }
    {}

    RPTKeywordNormalisation::MnemonicMap
    operator()(const std::vector<int>& controlValues) const;

private:
    std::vector<std::string> keywords_{};
};

} // namespace Opm

#endif // OPM_OPM_SIMPLE_RPT_INTEGER_CONTROL_HANDLER_HPP_INCLUDED
