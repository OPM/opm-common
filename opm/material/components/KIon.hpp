// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 NORCE.

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
 * \copydoc Opm::KIon
 */
#ifndef OPM_KION_HPP
#define OPM_KION_HPP

#include <opm/material/components/Component.hpp>

namespace Opm
{

/*!
 * \ingroup Components
 *
 * \brief Properties of K+ (potassium) ion
 *
 * \tparam Scalar The type used for scalar values
 */
template <class Scalar>
class KIon : public Component<Scalar, KIon<Scalar> >
{
public:
    /*!
     * \copydoc Component::name
     */
    static std::string_view name()
    {
        return "K+";
    }

    /*!
     * \copydoc Component::molarMass
     */
    static Scalar molarMass()
    {
        return 39.0983e-3;
    }
};

}

#endif //OPM_KION_HPP
