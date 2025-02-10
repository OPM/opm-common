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


#include "TwoPhaseLETCurves.hpp"
#include "TwoPhaseLETCurvesParams.hpp"
#include "PiecewiseLinearTwoPhaseMaterial.hpp"
#include "PiecewiseLinearTwoPhaseMaterialParams.hpp"

#include <opm/material/common/EnsureFinalized.hpp>

#include <type_traits>
#include <cassert>
#include <memory>

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
public:
    using Traits = TraitsT;
    using Scalar = typename TraitsT::Scalar;
    enum { numPhases = 2 };

private:
    using LETTwoPhaseLaw = TwoPhaseLETCurves<Traits>;
    using PLTwoPhaseLaw = PiecewiseLinearTwoPhaseMaterial<Traits>;

    using LETParams = typename LETTwoPhaseLaw::Params;
    using PLParams = typename PLTwoPhaseLaw::Params;

    template <class ParamT>
    struct Deleter
    {
        inline void operator () ( void* ptr )
        {
            delete static_cast< ParamT* > (ptr);
        }
    };

    using ParamPointerType = std::shared_ptr<void>;

public:

    /*!
     * \brief The multiplexer constructor.
     */
    SatCurveMultiplexerParams() : realParams_()
    {
    }

    SatCurveMultiplexerParams(const SatCurveMultiplexerParams& other)
        : realParams_()
    {
        setApproach( other.approach() );
    }

    SatCurveMultiplexerParams& operator= ( const SatCurveMultiplexerParams& other )
    {
        realParams_.reset();
        setApproach( other.approach() );
        return *this;
    }

    void setApproach(SatCurveMultiplexerApproach newApproach)
    {
        assert(realParams_ == 0);
        approach_ = newApproach;

        switch (approach()) {
        case SatCurveMultiplexerApproach::LET:
            realParams_ = ParamPointerType(new LETParams, Deleter< LETParams > () );
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinear:
            realParams_ = ParamPointerType(new PLParams, Deleter< PLParams > () );
            break;
        }
    }

    SatCurveMultiplexerApproach approach() const
    { return approach_; }

    // get the parameter object for the LET curve
    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::LET, LETParams>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<LETParams>();
    }

    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::LET, const LETParams>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<LETParams>();
    }

    // get the parameter object for the PL curve
    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::PiecewiseLinear, PLParams>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<PLParams>();
    }

    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::PiecewiseLinear, const PLParams>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<PLParams>();
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        switch (approach()) {
        case SatCurveMultiplexerApproach::LET:
            serializer(castTo<LETParams>());
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinear:
            serializer(castTo<PLParams>());
            break;
        }
    }

private:
    template <class ParamT>
    ParamT& castTo()
    {
        return *(static_cast<ParamT *> (realParams_.operator->()));
    }

    template <class ParamT>
    const ParamT& castTo() const
    {
        return *(static_cast<const ParamT *> (realParams_.operator->()));
    }

    SatCurveMultiplexerApproach approach_{SatCurveMultiplexerApproach::PiecewiseLinear};
    ParamPointerType realParams_;
};

} // namespace Opm

#endif // OPM_SAT_CURVE_MULTIPLEXER_PARAMS_HPP
