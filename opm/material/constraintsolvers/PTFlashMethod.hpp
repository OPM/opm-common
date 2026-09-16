// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
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
/*!
 * \file
 * \copydoc Opm::PTFlashMethod
 */
#ifndef OPM_PTFLASH_METHOD_HPP
#define OPM_PTFLASH_METHOD_HPP

#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Opm
{

/*!
 * \brief The nonlinear solver used for the two-phase vapour-liquid compositions.
 */
enum class PTFlashMethod {
    //! Successive substitution.
    Ssi,
    //! Newton's method.
    Newton,
    //! A few substitution steps to condition the Newton start, falling back
    //! to substitution should Newton fail.
    SsiNewton,
};

//! \brief The name the two-phase flash method is selected by on the command line.
inline std::string_view
ptFlashMethodToString(PTFlashMethod method)
{
    switch (method) {
    case PTFlashMethod::Ssi:
        return "ssi";
    case PTFlashMethod::Newton:
        return "newton";
    case PTFlashMethod::SsiNewton:
        return "ssi+newton";
    }

    throw std::invalid_argument("Unknown PTFlashMethod");
}

//! \brief The two-phase flash method of the given name.
inline PTFlashMethod
ptFlashMethodFromString(std::string_view str)
{
    if (str == "ssi") {
        return PTFlashMethod::Ssi;
    }
    if (str == "newton") {
        return PTFlashMethod::Newton;
    }
    if (str == "ssi+newton") {
        return PTFlashMethod::SsiNewton;
    }

    throw std::invalid_argument("Unknown two phase flash method " + std::string {str}
                                + " is specified. Available options are: ssi, newton, ssi+newton");
}

inline std::ostream&
operator<<(std::ostream& os, PTFlashMethod method)
{
    return os << ptFlashMethodToString(method);
}

} // namespace Opm

#endif // OPM_PTFLASH_METHOD_HPP
