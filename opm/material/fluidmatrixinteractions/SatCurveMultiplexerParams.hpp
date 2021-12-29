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

namespace Opm

{enum class SatCurveMultiplexerApproach {
    PiecewiseLinearApproach,
    LETApproach
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
    typedef TraitsT Traits;
    typedef typename TraitsT::Scalar Scalar;
    enum { numPhases = 2 };

private:
    typedef TwoPhaseLETCurves<Traits> LETTwoPhaseLaw;
    typedef PiecewiseLinearTwoPhaseMaterial<Traits> PLTwoPhaseLaw;

    typedef typename LETTwoPhaseLaw::Params LETParams;
    typedef typename PLTwoPhaseLaw::Params PLParams;

    template <class ParamT>
    struct Deleter
    {
        inline void operator () ( void* ptr )
        {
            delete static_cast< ParamT* > (ptr);
        }
    };

    typedef std::shared_ptr< void > ParamPointerType;

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
        case SatCurveMultiplexerApproach::LETApproach:
            realParams_ = ParamPointerType(new LETParams, Deleter< LETParams > () );
            break;

        case SatCurveMultiplexerApproach::PiecewiseLinearApproach:
            realParams_ = ParamPointerType(new PLParams, Deleter< PLParams > () );
            break;
        }
    }

    SatCurveMultiplexerApproach approach() const
    { return approach_; }

    // get the parameter object for the LET curve
    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::LETApproach, LETParams>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<LETParams>();
    }

    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::LETApproach, const LETParams>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<LETParams>();
    }

    // get the parameter object for the PL curve
    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::PiecewiseLinearApproach, PLParams>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<PLParams>();
    }

    template <SatCurveMultiplexerApproach approachV>
    typename std::enable_if<approachV == SatCurveMultiplexerApproach::PiecewiseLinearApproach, const PLParams>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<PLParams>();
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

    SatCurveMultiplexerApproach approach_;
    ParamPointerType realParams_;
};
} // namespace Opm

#endif // OPM_SAT_CURVE_MULTIPLEXER_PARAMS_HPP
