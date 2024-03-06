/*
  Copyright 2019  Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along with
  OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UDQPARSER_HPP
#define UDQPARSER_HPP

#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQToken.hpp>

#include <memory>
#include <string>
#include <vector>

namespace Opm {

class ErrorGuard;
class KeywordLocation;
class ParseContext;
class UDQASTNode;
class UDQParams;

} // namespace Opm

namespace Opm {

    std::unique_ptr<UDQASTNode>
    parseUDQExpression(const UDQParams&             udq_params,
                       UDQVarType                   target_type,
                       const std::string&           target_var,
                       const KeywordLocation&       location,
                       const std::vector<UDQToken>& tokens_,
                       const ParseContext&          parseContext,
                       ErrorGuard&                  errors);

} // namespace Opm

#endif  // UDQPARSER_HPP
