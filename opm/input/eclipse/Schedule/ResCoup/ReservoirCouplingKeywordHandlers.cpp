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

#include "ReservoirCouplingKeywordHandlers.hpp"
#include "ReservoirCouplingInfo.hpp"
#include "../HandlerContext.hpp"
#include "GrupSlav.hpp"
#include "Slaves.hpp"
#include "MasterGroup.hpp"

#include <fmt/format.h>

namespace Opm {

namespace {

} // anonymous namespace

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getReservoirCouplingHandlers()
{
    return {
        { "SLAVES", &handleSLAVES },
        { "GRUPMAST", &handleGRUPMAST},
        { "GRUPSLAV", &handleGRUPSLAV}
    };
}

} // namespace Opm
