// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
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

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::SatCurveMultiplexerParams
 */
#ifndef OPM_SAT_CURVE_MULTIPLEXER_PARAMS_HPP
#define OPM_SAT_CURVE_MULTIPLEXER_PARAMS_HPP

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/fluidmatrixinteractions/TwoPhaseLETCurves.hpp>
#include <opm/material/fluidmatrixinteractions/TwoPhaseLETCurvesParams.hpp>
#include <opm/material/fluidmatrixinteractions/PiecewiseLinearTwoPhaseMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/PiecewiseLinearTwoPhaseMaterialParams.hpp>

#include <opm/material/common/EnsureFinalized.hpp>

#include <variant>

namespace Opm {

enum class SatCurveMultiplexerApproach {
    PiecewiseLinear,
    LET
};

/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Specification of the material parameters for the
 *        saturation function multiplexer.
 *
 *\see SatCurveMultiplexer
 */
template <class TraitsT>
class SatCurveMultiplexerParams : public EnsureFinalized
{
private:
    using LETTwoPhaseLaw = TwoPhaseLETCurves<TraitsT>;
    using PLTwoPhaseLaw = PiecewiseLinearTwoPhaseMaterial<TraitsT>;

public:
    using Traits = TraitsT;
    using Scalar = typename TraitsT::Scalar;
    enum { numPhases = 2 };

    using LETParams = typename LETTwoPhaseLaw::Params;
    using PLParams = typename PLTwoPhaseLaw::Params;

    /*!
     * \brief The multiplexer constructor.
     */
    void setApproach(SatCurveMultiplexerApproach newApproach)
    {
        approach_ = newApproach;

        switch (approach()) {
        case SatCurveMultiplexerApproach::LET:
            realParams_ = LETParams{};
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinear:
            realParams_ = PLParams{};
            break;
        }
    }

    SatCurveMultiplexerApproach approach() const
    { return approach_; }

    //! \brief Mutable visit for a single type.
    //! \details Used during configuration stage
    template<class Function>
    void visit1(Function f)
    {
        std::visit(VisitorOverloadSet{f, [](auto&) {}}, realParams_);
    }

    //! \brief Immutable visit for all types using a visitor set.
    template<class VisitorSet>
    void visit(VisitorSet f) const
    {
        std::visit(f, realParams_);
    }

private:
    std::variant<PLParams,
                 LETParams> realParams_;

    SatCurveMultiplexerApproach approach_;
};

} // namespace Opm

#endif // OPM_SAT_CURVE_MULTIPLEXER_PARAMS_HPP
