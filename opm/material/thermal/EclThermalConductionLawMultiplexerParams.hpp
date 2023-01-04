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
 * \copydoc Opm::EclThermalConductionLawMultiplexerParams
 */
#ifndef OPM_ECL_THERMAL_CONDUCTION_LAW_MULTIPLEXER_PARAMS_HPP
#define OPM_ECL_THERMAL_CONDUCTION_LAW_MULTIPLEXER_PARAMS_HPP

#include "EclThconrLawParams.hpp"
#include "EclThcLawParams.hpp"

#include <opm/material/common/EnsureFinalized.hpp>

#include <cassert>
#include <stdexcept>

namespace Opm {

enum class EclThermalConductionApproach {
    Undefined,
    Thconr,    // keywords: THCONR, THCONSF
    Thc,       // keywords: THCROCK, THCOIL, THCGAS, THCWATER
    Null,      // (no keywords)
};

/*!
 * \brief The default implementation of a parameter object for the
 *        ECL thermal law.
 */
template <class ScalarT>
class EclThermalConductionLawMultiplexerParams : public EnsureFinalized
{
    using ParamPointerType = void*;

public:
    using Scalar = ScalarT;

    using ThconrLawParams = EclThconrLawParams<ScalarT>;
    using ThcLawParams = EclThcLawParams<ScalarT>;

    EclThermalConductionLawMultiplexerParams(const EclThermalConductionLawMultiplexerParams&) = default;

    EclThermalConductionLawMultiplexerParams()
    { thermalConductionApproach_ = EclThermalConductionApproach::Undefined; }

    ~EclThermalConductionLawMultiplexerParams()
    { destroy_(); }

    void setThermalConductionApproach(EclThermalConductionApproach newApproach)
    {
        destroy_();

        thermalConductionApproach_ = newApproach;
        switch (thermalConductionApproach()) {
        case EclThermalConductionApproach::Undefined:
            throw std::logic_error("Cannot set the approach for thermal conduction to 'undefined'!");

        case EclThermalConductionApproach::Thconr:
            realParams_ = new ThconrLawParams;
            break;

        case EclThermalConductionApproach::Thc:
            realParams_ = new ThcLawParams;
            break;

        case EclThermalConductionApproach::Null:
            realParams_ = nullptr;
            break;
        }
    }

    EclThermalConductionApproach thermalConductionApproach() const
    { return thermalConductionApproach_; }

    // get the parameter object for the THCONR case
    template <EclThermalConductionApproach approachV>
    typename std::enable_if<approachV == EclThermalConductionApproach::Thconr, ThconrLawParams>::type&
    getRealParams()
    {
        assert(thermalConductionApproach() == approachV);
        return *static_cast<ThconrLawParams*>(realParams_);
    }

    template <EclThermalConductionApproach approachV>
    typename std::enable_if<approachV == EclThermalConductionApproach::Thconr, const ThconrLawParams>::type&
    getRealParams() const
    {
        assert(thermalConductionApproach() == approachV);
        return *static_cast<const ThconrLawParams*>(realParams_);
    }

    // get the parameter object for the THC* case
    template <EclThermalConductionApproach approachV>
    typename std::enable_if<approachV == EclThermalConductionApproach::Thc, ThcLawParams>::type&
    getRealParams()
    {
        assert(thermalConductionApproach() == approachV);
        return *static_cast<ThcLawParams*>(realParams_);
    }

    template <EclThermalConductionApproach approachV>
    typename std::enable_if<approachV == EclThermalConductionApproach::Thc, const ThcLawParams>::type&
    getRealParams() const
    {
        assert(thermalConductionApproach() == approachV);
        return *static_cast<const ThcLawParams*>(realParams_);
    }

private:
    void destroy_()
    {
        switch (thermalConductionApproach()) {
        case EclThermalConductionApproach::Undefined:
            break;

        case EclThermalConductionApproach::Thconr:
            delete static_cast<ThconrLawParams*>(realParams_);
            break;

        case EclThermalConductionApproach::Thc:
            delete static_cast<ThcLawParams*>(realParams_);
            break;

        case EclThermalConductionApproach::Null:
            break;
        }

        thermalConductionApproach_ = EclThermalConductionApproach::Undefined;
    }

    EclThermalConductionApproach thermalConductionApproach_;
    ParamPointerType realParams_;
};

} // namespace Opm

#endif
