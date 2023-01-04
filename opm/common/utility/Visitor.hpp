/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/

#ifndef VISITOR_HPP
#define VISITOR_HPP

#include <string>
#include <variant>

namespace Opm {

//! \brief Helper struct for for generating visitor overload sets.
template<class... Ts>
struct VisitorOverloadSet : Ts...
{
    using Ts::operator()...;
};

//! \brief Deduction guide for visitor overload sets.
template<class... Ts> VisitorOverloadSet(Ts...) -> VisitorOverloadSet<Ts...>;

//! \brief A functor for handling a monostate in a visitor overload set.
//! \details Throws an exception
template<class Exception>
struct MonoThrowHandler {
    MonoThrowHandler(const std::string& message)
        : message_(message)
    {}

    void operator()(std::monostate&)
    {
        throw Exception(message_);
    }

    void operator()(const std::monostate&) const
    {
        throw Exception(message_);
    }

private:
    std::string message_;
};

}

#endif
