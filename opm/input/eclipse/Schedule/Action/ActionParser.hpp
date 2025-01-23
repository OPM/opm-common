/*
  Copyright 2019 Equinor ASA.

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

#ifndef ACTION_PARSER_HPP
#define ACTION_PARSER_HPP

#include <opm/input/eclipse/Schedule/Action/ASTNode.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionValue.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Opm::Action::Parser {
    /// Form expression evaluation tree from sequence of condition tokens.
    ///
    /// \param[in] tokens Sequence of concatenated condition string tokens
    /// of a single ACTIONX condition block from which to form an expression
    /// evaluation tree.  Newline characters and other whitespace assumed to
    /// be removed.
    [[nodiscard]] std::unique_ptr<ASTNode>
    parseCondition(const std::vector<std::string>& tokens);

    /// Classify an Action condition string token.
    ///
    /// \param[in] arg Action condition string token.
    ///
    /// \return Kind of token contained in \p arg, e.g., an expression, an
    /// operator or a parenthesis.
    [[nodiscard]] TokenType tokenType(const std::string& arg);
} // namespace Opm::Action::Parser

#endif // ACTION_PARSER_HPP
