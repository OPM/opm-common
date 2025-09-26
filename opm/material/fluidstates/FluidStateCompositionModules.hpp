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
 *
 * \brief Modules for the ModularFluidState which represent composition.
 */
#ifndef OPM_FLUID_STATE_COMPOSITION_MODULES_HPP
#define OPM_FLUID_STATE_COMPOSITION_MODULES_HPP

#include <opm/material/common/Valgrind.hpp>
#include <opm/material/common/MathToolbox.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace Opm {

/*!
 * \brief Module for the modular fluid state which stores the
 *        phase compositions explicitly in terms of mole fractions.
 */
template <class Scalar,
          class FluidSystem,
          class Implementation>
class FluidStateExplicitCompositionModule
{
    enum { numPhases = FluidSystem::numPhases };
    enum { numComponents = FluidSystem::numComponents };

public:
    FluidStateExplicitCompositionModule()
    {
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx)
            for (int compIdx = 0; compIdx < numComponents; ++compIdx)
                moleFraction_[phaseIdx][compIdx] = 0.0;
        // TODO: possilby we should begin with totalMoleFractions_ here
        // the current implementation assuming we have the moleFractions_ for each
        // individual phases first.

        Valgrind::SetDefined(moleFraction_);
        Valgrind::SetUndefined(averageMolarMass_);
        Valgrind::SetUndefined(sumMoleFractions_);
    }

    /*!
     * \brief The mole fraction of a component in a phase []
     */
    const Scalar& moleFraction(unsigned phaseIdx, unsigned compIdx) const
    { return moleFraction_[phaseIdx][compIdx]; }

    /*!
     * \brief The total mole fraction of a component []
     */
    const Scalar& moleFraction(unsigned compIdx) const
    { return totalModelFractions_[compIdx]; }

    /*!
     * \brief The mass fraction of a component in a phase []
     */
    Scalar massFraction(unsigned phaseIdx, unsigned compIdx) const
    {
        return
            abs(sumMoleFractions_[phaseIdx])
            *moleFraction_[phaseIdx][compIdx]
            *FluidSystem::molarMass(compIdx)
            / max(1e-40, abs(averageMolarMass_[phaseIdx]));
    }

    /*!
     * \brief The mean molar mass of a fluid phase [kg/mol]
     *
     * The average molar mass is the mean mass of one mole of the
     * fluid at current composition. It is defined as the sum of the
     * component's molar masses weighted by the current mole fraction:
     * \f[ \bar M_\alpha = \sum_\kappa M^\kappa x_\alpha^\kappa \f]
     */
    const Scalar& averageMolarMass(unsigned phaseIdx) const
    { return averageMolarMass_[phaseIdx]; }

    /*!
     * \brief The concentration of a component in a phase [mol/m^3]
     *
     * This quantity is often called "molar concentration" or just
     * "concentration", but there are many other (though less common)
     * measures for concentration.
     *
     * http://en.wikipedia.org/wiki/Concentration
     */
    Scalar molarity(unsigned phaseIdx, unsigned compIdx) const
    { return asImp_().molarDensity(phaseIdx)*moleFraction(phaseIdx, compIdx); }

    /*!
     * \brief Set the mole fraction of a component  in a phase []
     *        and update the average molar mass [kg/mol] according
     *        to the current composition of the phase
     */
    void setMoleFraction(unsigned phaseIdx, unsigned compIdx, const Scalar& value)
    {
        Valgrind::CheckDefined(value);
        Valgrind::SetUndefined(sumMoleFractions_[phaseIdx]);
        Valgrind::SetUndefined(averageMolarMass_[phaseIdx]);
        Valgrind::SetUndefined(moleFraction_[phaseIdx][compIdx]);

        moleFraction_[phaseIdx][compIdx] = value;

        // re-calculate the mean molar mass
        sumMoleFractions_[phaseIdx] = 0.0;
        averageMolarMass_[phaseIdx] = 0.0;
        for (unsigned compJIdx = 0; compJIdx < numComponents; ++compJIdx) {
            sumMoleFractions_[phaseIdx] += moleFraction_[phaseIdx][compJIdx];
            averageMolarMass_[phaseIdx] += moleFraction_[phaseIdx][compJIdx]*FluidSystem::molarMass(compJIdx);
        }
    }

    /*!
     * \brief Set the total mole fraction of a component
     */
    void setMoleFraction(unsigned compIdx, const Scalar& value)
    {
        Valgrind::CheckDefined(value);
        totalModelFractions_[compIdx] = value;
    }

    void setCompressFactor(unsigned phaseIdx, const Scalar& value) {
        Valgrind::CheckDefined(value);
        Z_[phaseIdx] = value;
    }

    Scalar compressFactor(unsigned phaseIdx) const {
        return Z_[phaseIdx];
    }

    /*!
     * \brief Retrieve all parameters from an arbitrary fluid
     *        state.
     */
    template <class FluidState>
    void assign(const FluidState& fs)
    {
        for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            averageMolarMass_[phaseIdx] = 0;
            sumMoleFractions_[phaseIdx] = 0;
            for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
                moleFraction_[phaseIdx][compIdx] =
                    decay<Scalar>(fs.moleFraction(phaseIdx, compIdx));

                averageMolarMass_[phaseIdx] += moleFraction_[phaseIdx][compIdx]*FluidSystem::molarMass(compIdx);
                sumMoleFractions_[phaseIdx] += moleFraction_[phaseIdx][compIdx];
            }
        }
    }

    /*!
     * \brief Make sure that all attributes are defined.
     *
     * This method does not do anything if the program is not run
     * under valgrind. If it is, then valgrind will print an error
     * message if some attributes of the object have not been properly
     * defined.
     */
    void checkDefined() const
    {
        Valgrind::CheckDefined(moleFraction_);
        Valgrind::CheckDefined(averageMolarMass_);
        Valgrind::CheckDefined(sumMoleFractions_);
        Valgrind::CheckDefined(K_);
        Valgrind::CheckDefined(L_);
    }

    const Scalar& K(unsigned compIdx) const
    {
        return K_[compIdx];
    }

    /*!
     * \brief Set the K value of a component [-]
     */
    void setKvalue(unsigned compIdx, const Scalar& value)
    {
        K_[compIdx] = value;
    }

    /*!
     * \brief The L value of a composition [-]
     */
    const Scalar& L() const
    {
        return L_;
    }

    /*!
     * \brief Set the L value [-]
     */
    void setLvalue(const Scalar& value)
    {
        L_ = value;
    }

    /*!
    * \brief Wilson formula to calculate K
    *
    */
    Scalar wilsonK_(unsigned compIdx) const
    {
        const auto& acf = FluidSystem::acentricFactor(compIdx);
        const auto& T_crit = FluidSystem::criticalTemperature(compIdx);
        const auto& T = asImp_().temperature(0);
        const auto& p_crit = FluidSystem::criticalPressure(compIdx);
        const auto& p = asImp_().pressure(0); //for now assume no capillary pressure

        const auto tmp = exp(5.37 * (1+acf) * (1-T_crit/T)) * (p_crit/p);
        return tmp;
    }

protected:
    const Implementation& asImp_() const
    {
        return *static_cast<const Implementation*>(this);
    }

    std::array<std::array<Scalar,numComponents>,numPhases> moleFraction_{};
    std::array<Scalar,numPhases> averageMolarMass_{};
    std::array<Scalar,numPhases> sumMoleFractions_{}; // per phase
    std::array<Scalar, numComponents> totalModelFractions_{}; // total mole fractions for each component
    std::array<Scalar,numPhases> Z_{};
    std::array<Scalar,numComponents> K_{};
    Scalar L_{};
};

/*!
 * \brief Module for the modular fluid state which provides the
 *        phase compositions assuming immiscibility.
 */
template <class Scalar,
          class FluidSystem,
          class Implementation>
class FluidStateImmiscibleCompositionModule
{
    enum { numPhases = FluidSystem::numPhases };

public:
    enum { numComponents = FluidSystem::numComponents };
    static_assert(static_cast<int>(numPhases) == static_cast<int>(numComponents),
                  "The number of phases must be the same as the number of (pseudo-) components if you assume immiscibility");

    FluidStateImmiscibleCompositionModule() = default;

    /*!
     * \brief The mole fraction of a component in a phase []
     */
    Scalar moleFraction(unsigned phaseIdx, unsigned compIdx) const
    { return (phaseIdx == compIdx)?1.0:0.0; }

    /*!
     * \brief The mass fraction of a component in a phase []
     */
    Scalar massFraction(unsigned phaseIdx, unsigned compIdx) const
    { return (phaseIdx == compIdx)?1.0:0.0; }

    /*!
     * \brief The mean molar mass of a fluid phase [kg/mol]
     *
     * The average mass is the mean molar mass of a molecule of the
     * fluid at current composition. It is defined as the sum of the
     * component's molar masses weighted by the current mole fraction:
     * \f[ \bar M_\alpha = \sum_\kappa M^\kappa x_\alpha^\kappa \f]
     */
    Scalar averageMolarMass(unsigned phaseIdx) const
    { return FluidSystem::molarMass(/*compIdx=*/phaseIdx); }

    /*!
     * \brief The concentration of a component in a phase [mol/m^3]
     *
     * This quantity is often called "molar concentration" or just
     * "concentration", but there are many other (though less common)
     * measures for concentration.
     *
     * http://en.wikipedia.org/wiki/Concentration
     */
    Scalar molarity(unsigned phaseIdx, unsigned compIdx) const
    { return asImp_().molarDensity(phaseIdx)*moleFraction(phaseIdx, compIdx); }

    /*!
     * \brief Retrieve all parameters from an arbitrary fluid
     *        state.
     */
    template <class FluidState>
    void assign(const FluidState& /* fs */)
    { }

    /*!
     * \brief Make sure that all attributes are defined.
     *
     * This method does not do anything if the program is not run
     * under valgrind. If it is, then valgrind will print an error
     * message if some attributes of the object have not been properly
     * defined.
     */
    void checkDefined() const
    { }

protected:
    const Implementation& asImp_() const
    { return *static_cast<const Implementation*>(this); }
};

/*!
 * \brief Module for the modular fluid state which does not store the
 *        compositions but throws std::logic_error instead.
 */
template <class Scalar>
class FluidStateNullCompositionModule
{
public:
    enum { numComponents = 0 };

    FluidStateNullCompositionModule() = default;

    /*!
     * \brief The mole fraction of a component in a phase []
     */
    Scalar moleFraction(unsigned /* phaseIdx */, unsigned /* compIdx */) const
    { throw std::logic_error("Mole fractions are not provided by this fluid state"); }

    /*!
     * \brief The mass fraction of a component in a phase []
     */
    Scalar massFraction(unsigned /* phaseIdx */, unsigned /* compIdx */) const
    { throw std::logic_error("Mass fractions are not provided by this fluid state"); }

    /*!
     * \brief The mean molar mass of a fluid phase [kg/mol]
     *
     * The average mass is the mean molar mass of a molecule of the
     * fluid at current composition. It is defined as the sum of the
     * component's molar masses weighted by the current mole fraction:
     * \f[ \bar M_\alpha = \sum_\kappa M^\kappa x_\alpha^\kappa \f]
     */
    Scalar averageMolarMass(unsigned /* phaseIdx */) const
    { throw std::logic_error("Mean molar masses are not provided by this fluid state"); }

    /*!
     * \brief The concentration of a component in a phase [mol/m^3]
     *
     * This quantity is often called "molar concentration" or just
     * "concentration", but there are many other (though less common)
     * measures for concentration.
     *
     * http://en.wikipedia.org/wiki/Concentration
     */
    Scalar molarity(unsigned /* phaseIdx */, unsigned /* compIdx */) const
    { throw std::logic_error("Molarities are not provided by this fluid state"); }

    /*!
     * \brief Make sure that all attributes are defined.
     *
     * This method does not do anything if the program is not run
     * under valgrind. If it is, then valgrind will print an error
     * message if some attributes of the object have not been properly
     * defined.
     */
    void checkDefined() const
    { }
};


} // namespace Opm

#endif
