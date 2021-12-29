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
 * \copydoc Opm::SatCurveMultiplexer
 */
#ifndef OPM_SAT_CURVE_MULTIPLEXER_HPP
#define OPM_SAT_CURVE_MULTIPLEXER_HPP

#include "SatCurveMultiplexerParams.hpp"

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Exceptions.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>

namespace Opm {
/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Implements a multiplexer class that provides LET curves
 *        and piecewise linear saturation functions.
 *
 */
template <class TraitsT, class ParamsT = SatCurveMultiplexerParams<TraitsT> >
class SatCurveMultiplexer : public TraitsT
{
public:
    typedef TraitsT Traits;
    typedef ParamsT Params;
    typedef typename Traits::Scalar Scalar;

    typedef TwoPhaseLETCurves<Traits> LETTwoPhaseLaw;
    typedef PiecewiseLinearTwoPhaseMaterial<Traits> PLTwoPhaseLaw;

    //! The number of fluid phases to which this material law applies.
    static const int numPhases = Traits::numPhases;
    static_assert(numPhases == 2,
                  "The Brooks-Corey capillary pressure law only applies "
                  "to the case of two fluid phases");

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

    static_assert(Traits::numPhases == 2,
                  "The number of fluid phases must be two if you want to use "
                  "this material law!");

    /*!
     * \brief The capillary pressure-saturation curves.
     */
    template <class Container, class FluidState>
    static void capillaryPressures(Container& values, const Params& params, const FluidState& fluidState)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            LETTwoPhaseLaw::capillaryPressures(values,
                                               params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                               fluidState);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            PLTwoPhaseLaw::capillaryPressures(values,
                                              params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                              fluidState);
            break;
        }
    }

    /*!
     * \brief Calculate the saturations of the phases starting from
     *        their pressure differences.
     */
    template <class Container, class FluidState>
    static void saturations(Container& values, const Params& params, const FluidState& fluidState)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            LETTwoPhaseLaw::saturations(values,
                                        params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                        fluidState);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            PLTwoPhaseLaw::saturations(values,
                                       params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                       fluidState);
            break;
        }
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
    static void relativePermeabilities(Container& values, const Params& params, const FluidState& fluidState)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            LETTwoPhaseLaw::relativePermeabilities(values,
                                                   params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                                   fluidState);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            PLTwoPhaseLaw::relativePermeabilities(values,
                                                  params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                                  fluidState);
            break;
        }
    }

    /*!
     * \brief The capillary pressure-saturation curve
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation pcnw(const Params& params, const FluidState& fluidState)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::pcnw(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                        fluidState);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::pcnw(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                       fluidState);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatPcnw(const Params& params, const Evaluation& Sw)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::twoPhaseSatPcnw(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                                   Sw);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::twoPhaseSatPcnw(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                                  Sw);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatPcnwInv(const Params& params, const Evaluation& pcnw)
    {
        throw std::logic_error("SatCurveMultiplexer::twoPhaseSatPcnwInv"
                               " not implemented!");
    }

    /*!
     * \brief The saturation-capillary pressure curve
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation Sw(const Params& params, const FluidState& fluidstate)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::Sw(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                      fluidstate);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::Sw(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                     fluidstate);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatSw(const Params& params, const Evaluation& pc)
    {
        throw std::logic_error("SatCurveMultiplexer::twoPhaseSatSw"
                               " not implemented!");
    }

    /*!
     * \brief Calculate the non-wetting phase saturations depending on
     *        the phase pressures.
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation Sn(const Params& params, const FluidState& fluidstate)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::Sn(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                      fluidstate);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::Sn(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                     fluidstate);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatSn(const Params& params, const Evaluation& pc)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::twoPhaseSatSn(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                                 pc);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::twoPhaseSatSn(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                                pc);
            break;
        }

        return 0.0;
    }

    /*!
     * \brief The relative permeability for the wetting phase of
     *        the medium
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation krw(const Params& params, const FluidState& fluidstate)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::krw(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                       fluidstate);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::krw(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                      fluidstate);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrw(const Params& params, const Evaluation& Sw)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::twoPhaseSatKrw(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                                  Sw);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::twoPhaseSatKrw(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                                 Sw);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrwInv(const Params& params, const Evaluation& krw)
    {
        throw std::logic_error("Not implemented: twoPhaseSatKrwInv()");
    }

    /*!
     * \brief The relative permeability for the non-wetting phase of
     *        the medium
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation krn(const Params& params, const FluidState& fluidstate)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::krn(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                       fluidstate);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::krn(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                      fluidstate);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrn(const Params& params, const Evaluation& Sw)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::twoPhaseSatKrn(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                                  Sw);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::twoPhaseSatKrn(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                                 Sw);
            break;
        }

        return 0.0;
    }

    template <class Evaluation>
    static Evaluation twoPhaseSatKrnInv(const Params& params, const Evaluation& krn)
    {
        switch (params.approach()) {
        case SatCurveMultiplexerApproach::LETApproach:
            return LETTwoPhaseLaw::twoPhaseSatKrnInv(params.template getRealParams<SatCurveMultiplexerApproach::LETApproach>(),
                                                     krn);
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            return PLTwoPhaseLaw::twoPhaseSatKrnInv(params.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinearApproach>(),
                                                    krn);
            break;
        }

        return 0.0;
    }
};
} // namespace Opm

#endif // OPM_SAT_CURVE_MULTIPLEXER_HPP
