// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2025 NORCE AS

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
#ifndef TPSA_MATERIAL_STATE_HPP
#define TPSA_MATERIAL_STATE_HPP

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Valgrind.hpp>

#include <array>


namespace Opm {

template <class Scalar>
class MaterialStateTPSA
{
public:
    /*!
    * \brief Constructor
    */
    MaterialStateTPSA()
    {
        Opm::Valgrind::SetUndefined(displacement_);
        Opm::Valgrind::SetUndefined(rotation_);
        Opm::Valgrind::SetUndefined(solidPressure_);
    }

    /*!
    * \brief Return direction (x-, y- or z-) component of displacement
    *
    * \param dirIdx Direction component index
    */
    const Scalar displacement(unsigned dirIdx) const
    {
        return displacement_[dirIdx];
    }

    /*!
    * \brief Return direction (x-, y- or z-) component of rotation
    *
    * \param dirIdx Direction component index
    */
    const Scalar rotation(unsigned dirIdx) const
    {
        return rotation_[dirIdx];
    }

    /*!
    * \brief Return solid pressure
    */
    const Scalar solidPressure() const
    {
        return solidPressure_;
    }

    /*!
    * \brief Set a direction (x-, y- or z-) component of displacement
    *
    * \param dirIdx Direction component index
    * \param value Displacement value
    */
    void setDisplacement(unsigned dirIdx, const Scalar value)
    {
        Opm::Valgrind::CheckDefined(value);
        Opm::Valgrind::SetUndefined(displacement_[dirIdx]);

        displacement_[dirIdx] = value;
    }

    /*!
    * \brief Set a direction (x-, y- or z-) component of rotation
    *
    * \param dirIdx Direction component index
    * \param value Rotation value
    */
    void setRotation(unsigned dirIdx, const Scalar value)
    {
        Opm::Valgrind::CheckDefined(value);
        Opm::Valgrind::SetUndefined(rotation_[dirIdx]);

        rotation_[dirIdx] = value;
    }

    /*!
    * \brief Set solid pressure
    *
    * \param value Solid pressure value
    */
    void setSolidPressure(const Scalar value)
    {
        Opm::Valgrind::CheckDefined(value);
        Opm::Valgrind::SetUndefined(solidPressure_);

        solidPressure_ = value;
    }

    /*!
    * \brief Assign from another material state container
    *
    * \param ms Incoming material state container
    */
    template <class MaterialState>
    void assign(const MaterialState& ms)
    {
        // Assign displacement and rotation
        for (unsigned dirIdx = 0; dirIdx < 3; ++dirIdx) {
            displacement_[dirIdx] = Opm::decay<Scalar>(ms.displacement(dirIdx));
            rotation_[dirIdx] = Opm::decay<Scalar>(ms.rotation(dirIdx));
        }

        // Assign solid pressure
        solidPressure_ = Opm::decay<Scalar>(ms.solidPressure());
    }

    /*!
    * \brief Instruct Valgrind to check the definedness of all attributes of this class
    */
    void checkDefined() const
    {
        Opm::Valgrind::CheckDefined(displacement_);
        Opm::Valgrind::CheckDefined(rotation_);
        Opm::Valgrind::CheckDefined(solidPressure_);
    }

protected:
    std::array<Scalar, 3> displacement_{};
    std::array<Scalar, 3> rotation_{};
    Scalar solidPressure_{};
};  // class MaterialState

}  // namespace Opm

#endif
