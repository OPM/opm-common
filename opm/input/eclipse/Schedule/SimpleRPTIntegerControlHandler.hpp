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

/// Report keyword integer control handler from sequence of mnemonic
/// strings.
class SimpleRPTIntegerControlHandler
{
public:
    /// Constructor.
    ///
    /// Internalises sequence of mnemonic strings from which to infer an
    /// integer control handler.
    ///
    /// \param[in] keywords Mnemonic strings.
    explicit SimpleRPTIntegerControlHandler(std::initializer_list<const char*> keywords)
        : keywords_ { keywords.begin(), keywords.end() }
    {}

    /// Function call operator.
    ///
    /// Creates sequence of mnemonics along with associate integer mnemonic
    /// values.
    ///
    /// \param[in] controlValues Integer control values.
    ///
    /// \return Mnemonics and their associate integer values.
    RPTKeywordNormalisation::MnemonicMap
    operator()(const std::vector<int>& controlValues) const;

private:
    /// Sequence of mnemonic strings.  Defined by user's constructor call.
    std::vector<std::string> keywords_{};
};

} // namespace Opm

#endif // OPM_OPM_SIMPLE_RPT_INTEGER_CONTROL_HANDLER_HPP_INCLUDED
