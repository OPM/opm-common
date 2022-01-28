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
 * \copydoc Opm::LETCurves
 */
#ifndef OPM_TWO_PHASE_LET_CURVES_HPP
#define OPM_TWO_PHASE_LET_CURVES_HPP

#include "TwoPhaseLETCurvesParams.hpp"

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Exceptions.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>

namespace Opm {
/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Implementation of the LET curve saturation functions.
 *
 * This class provides the "raw" curves as static members and
 * relies on the class EffToAbsLaw for converting absolute to
 * effective saturations and vice versa.
 *
 *\see LETCurvesParams
 */
template <class TraitsT, class ParamsT = TwoPhaseLETCurvesParams<TraitsT> >
class TwoPhaseLETCurves : public TraitsT
{
public:
    typedef TraitsT Traits;
    typedef ParamsT Params;
    typedef typename Traits::Scalar Scalar;

    static_assert(Traits::numPhases == 2,
                  "The number of fluid phases must be two if you want to use "
                  "this material law!");

    static constexpr Scalar eps = 1.0e-10; //tolerance

    //! The number of fluid phases to which this material law applies.
    static const int numPhases = Traits::numPhases;

    //! Specify whether this material law implements the two-phase
    //! convenience API
    static const bool implementsTwoPhaseApi = true;

    //! Specify whether this material law implements the two-phase
    //! convenience API which only depends on the phase saturations
    static const bool implementsTwoPhaseSatApi = true;

    //! Specify whether the quantities defined by this material law
    //! are saturation dependent
    static const bool isSaturationDependent = true;

    //! Specify whether the quantities defined by this material law
    //! are dependent on the absolute pressure
    static const bool isPressureDependent = false;

    //! Specify whether the quantities defined by this material law
    //! are temperature dependent
    static const bool isTemperatureDependent = false;

    //! Specify whether the quantities defined by this material law
    //! are dependent on the phase composition
    static const bool isCompositionDependent = false;

    /*!
     * \brief The capillary pressure-saturation curves.
     */
    template <class Container, class FluidState>
    static void capillaryPressures(Container& /* values */, const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::invalid_argument("The capillaryPressures(fs) method is not yet implemented");
    }

    /*!
     * \brief Calculate the saturations of the phases starting from
     *        their pressure differences.
     */
    template <class Container, class FluidState>
    static void saturations(Container& /* pc */, const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::invalid_argument("The saturations(fs) method is not yet implemented");
    }

    /*!
     * \brief The relative permeability-saturation curves.
     *
     * \param values A random access container which stores the
     *               relative permeability of each fluid phase.
     * \param params The parameter object expressing the coefficients
     *               required by the material law.
     * \param fs The fluid state for which the relative permeabilities
     *           ought to be calculated
     */
    template <class Container, class FluidState>
    static void relativePermeabilities(Container& /* pc */, const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::invalid_argument("The relativePermeabilities(fs) method is not yet implemented");
    }

    /*!
     * \brief The capillary pressure-saturation curve
     *
     * \param params The parameters of the capillary pressure curve
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation pcnw(const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::logic_error("TwoPhaseLETCurves::pcnw"
                               " not implemented!");

    }

    template <class Evaluation>
    static Evaluation twoPhaseSatPcnw(const Params& params, const Evaluation& Sw)
    {
        Evaluation Ss = (Sw-params.Sminpc())/params.dSpc();
        if (Ss < 0.0) {
            Ss -= (Opm::decay<Scalar>(Ss));
        } else if (Ss > 1.0) {
            Ss -= (Opm::decay<Scalar>(Ss-1.0));
        }

        const Evaluation powS = Opm::pow(Ss,params.Tpc());
        const Evaluation pow1mS = Opm::pow(1.0-Ss,params.Lpc());

        const Evaluation F = pow1mS/(pow1mS+powS*params.Epc());
        Evaluation tmp = params.Pct()+(params.Pcir()-params.Pct())*F;

        return tmp;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatPcnwInv(const Params& /* params */, const Evaluation& pcnw)
    {
        throw std::logic_error("TwoPhaseLETCurves::twoPhaseSatPcnwInv"
                               " not implemented!");
    }

    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation Sw(const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::invalid_argument("The Sw(fs) method is not yet implemented");
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatSw(const Params& /* params */, const Evaluation& /* pc */)
    {
        throw std::invalid_argument("The twoPhaseSatSw(fs) method is not yet implemented");
    }

    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation Sn(const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::invalid_argument("The Sn(fs) method is not yet implemented");
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatSn(const Params& /* params */, const Evaluation& /* pc */)
    {
        throw std::invalid_argument("The twoPhaseSatSn(fs) method is not yet implemented");
    }
    /*!
     * \brief The relative permeability for the wetting phase of
     *        the medium implied by the LET parameterization.
     *
     * \param params The parameters of the relative permeability curve
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation krw(const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::logic_error("TwoPhaseLETCurves::krw"
                               " not implemented!");
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrw(const Params& params, const Evaluation& Sw)
    {
        return twoPhaseSatKrLET(Params::wIdx, params, Sw);
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrLET(const unsigned phaseIdx, const Params& params, const Evaluation& S)
    {
        Evaluation Ss = (S-params.Smin(phaseIdx))/params.dS(phaseIdx);
        if (Ss < 0.0) {
            Ss -= (Opm::decay<Scalar>(Ss));
        } else if (Ss > 1.0) {
            Ss -= (Opm::decay<Scalar>(Ss-1.0));
        }

        const Evaluation powS = Opm::pow(Ss,params.L(phaseIdx));
        const Evaluation pow1mS = Opm::pow(1.0-Ss,params.T(phaseIdx));

        const Evaluation tmp = params.Krt(phaseIdx)*powS/(powS+pow1mS*params.E(phaseIdx));

        return tmp;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrwInv(const Params& /* params */, const Evaluation& /* krw */)
    {
        throw std::logic_error("TwoPhaseLETCurves::twoPhaseSatKrwInv"
                               " not implemented!");
    }

    /*!
     * \brief The relative permeability for the non-wetting phase of
     *        the medium as implied by the LET parameterization.
     *
     * \param params The parameters of the capillary pressure curve
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation krn(const Params& /* params */, const FluidState& /* fs */)
    {
        throw std::logic_error("TwoPhaseLETCurves::krn"
                               " not implemented!");
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrn(const Params& params, const Evaluation& Sw)
    {
        const Evaluation Sn = 1.0 - Sw;

        return twoPhaseSatKrLET(Params::nwIdx, params, Sn);
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrnInv(const Params& params, const Evaluation& krn)
    {
        // since inverting the formula for krn is hard to do analytically, we use the
        // Newton-Raphson method
        Evaluation Sw = 0.5;
        //Scalar eps = 1e-10;
        for (int i = 0; i < 20; ++i) {
            Evaluation f = krn - twoPhaseSatKrn(params, Sw);
            if (Opm::abs(f) < 1e-10)
                return Sw;
            Evaluation fStar = krn - twoPhaseSatKrn(params, Sw + eps);
            Evaluation fPrime = (fStar - f)/eps;
            Evaluation delta = f/fPrime;

            Sw -= delta;
            if (Sw < 0)
                Sw = 0.0;
            if (Sw > 1.0)
                Sw = 1.0;
            if (Opm::abs(delta) < 1e-10)
                return Sw;
        }

        // Fallback to simple bisection
        Evaluation SL = 0.0;
        Evaluation fL = krn - twoPhaseSatKrn(params, SL);
        if (Opm::abs(fL) < eps)
            return SL;
        Evaluation SR = 1.0;
        Evaluation fR = krn - twoPhaseSatKrn(params, SR);
        if (Opm::abs(fR) < eps)
            return SR;
        if (fL*fR < 0.0) {
            for (int i = 0; i < 50; ++i) {
                Sw = 0.5*(SL+SR);
                if (abs(SR-SL) < eps)
                    return Sw;
                Evaluation fw = krn - twoPhaseSatKrn(params, Sw);
                if (Opm::abs(fw) < eps)
                    return Sw;
                if (fw * fR > 0) {
                    SR = Sw;
                    fR = fw;
                } else if (fw * fL > 0) {
                    SL = Sw;
                    fL = fw;
                }
            }

        }

        throw NumericalIssue("Couldn't invert the TwoPhaseLETCurves non-wetting phase"
                               " relperm within 20 newton iterations and 50 bisection iterations");
    }
};
} // namespace Opm

#endif // OPM_TWO_PHASE_LET_CURVES_HPP
