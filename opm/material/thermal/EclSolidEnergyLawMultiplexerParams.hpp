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
 * \copydoc Opm::EclSolidEnergyLawMultiplexerParams
 */
#ifndef OPM_ECL_SOLID_ENERGY_LAW_MULTIPLEXER_PARAMS_HPP
#define OPM_ECL_SOLID_ENERGY_LAW_MULTIPLEXER_PARAMS_HPP

#include "EclHeatcrLawParams.hpp"
#include "EclSpecrockLawParams.hpp"

#include <opm/material/common/EnsureFinalized.hpp>

#include <cassert>
#include <stdexcept>
#include <type_traits>

namespace Opm {

enum class EclSolidEnergyApproach {
    Undefined,
    Heatcr,   // keywords: HEATCR, HEATCRT, STCOND
    Specrock, // keyword: SPECROCK
    Null      // (no keywords)
};

/*!
 * \brief The default implementation of a parameter object for the
 *        ECL thermal law.
 */
template <class ScalarT>
class EclSolidEnergyLawMultiplexerParams : public EnsureFinalized
{
    using ParamPointerType = void*;

public:
    using Scalar = ScalarT;

    using HeatcrLawParams = EclHeatcrLawParams<ScalarT>;
    using SpecrockLawParams = EclSpecrockLawParams<ScalarT>;

    EclSolidEnergyLawMultiplexerParams(const EclSolidEnergyLawMultiplexerParams&) = default;

    EclSolidEnergyLawMultiplexerParams()
    { solidEnergyApproach_ = EclSolidEnergyApproach::Undefined; }

    ~EclSolidEnergyLawMultiplexerParams()
    { destroy_(); }

    void setSolidEnergyApproach(EclSolidEnergyApproach newApproach)
    {
        destroy_();

        solidEnergyApproach_ = newApproach;
        switch (solidEnergyApproach()) {
        case EclSolidEnergyApproach::Undefined:
            throw std::logic_error("Cannot set the approach for solid energy storage to 'undefined'!");

        case EclSolidEnergyApproach::Heatcr:
            realParams_ = new HeatcrLawParams;
            break;

        case EclSolidEnergyApproach::Specrock:
            realParams_ = new SpecrockLawParams;
            break;

        case EclSolidEnergyApproach::Null:
            realParams_ = nullptr;
            break;
        }
    }

    EclSolidEnergyApproach solidEnergyApproach() const
    { return solidEnergyApproach_; }

    // get the parameter object for the HEATCR case
    template <EclSolidEnergyApproach approachV>
    typename std::enable_if<approachV == EclSolidEnergyApproach::Heatcr, HeatcrLawParams>::type&
    getRealParams()
    {
        assert(solidEnergyApproach() == approachV);
        return *static_cast<HeatcrLawParams*>(realParams_);
    }

    template <EclSolidEnergyApproach approachV>
    typename std::enable_if<approachV == EclSolidEnergyApproach::Heatcr, const HeatcrLawParams>::type&
    getRealParams() const
    {
        assert(solidEnergyApproach() == approachV);
        return *static_cast<const HeatcrLawParams*>(realParams_);
    }

    // get the parameter object for the SPECROCK case
    template <EclSolidEnergyApproach approachV>
    typename std::enable_if<approachV == EclSolidEnergyApproach::Specrock, SpecrockLawParams>::type&
    getRealParams()
    {
        assert(solidEnergyApproach() == approachV);
        return *static_cast<SpecrockLawParams*>(realParams_);
    }

    template <EclSolidEnergyApproach approachV>
    typename std::enable_if<approachV == EclSolidEnergyApproach::Specrock, const SpecrockLawParams>::type&
    getRealParams() const
    {
        assert(solidEnergyApproach() == approachV);
        return *static_cast<const SpecrockLawParams*>(realParams_);
    }

private:
    void destroy_()
    {
        switch (solidEnergyApproach()) {
        case EclSolidEnergyApproach::Undefined:
            break;

        case EclSolidEnergyApproach::Heatcr:
            delete static_cast<HeatcrLawParams*>(realParams_);
            break;

        case EclSolidEnergyApproach::Specrock:
            delete static_cast<SpecrockLawParams*>(realParams_);
            break;

        case EclSolidEnergyApproach::Null:
            break;
        }

        solidEnergyApproach_ = EclSolidEnergyApproach::Undefined;
    }

    EclSolidEnergyApproach solidEnergyApproach_;
    ParamPointerType realParams_;
};

} // namespace Opm

#endif
