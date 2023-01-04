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

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/thermal/EclThconrLawParams.hpp>
#include <opm/material/thermal/EclThcLawParams.hpp>
#include <opm/material/thermal/NullThermalConductionLawParams.hpp>

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
template <class ScalarT, class FluidSystem>
class EclThermalConductionLawMultiplexerParams : public EnsureFinalized
{
public:
    using Scalar = ScalarT;

    using ThconrLawParams = EclThconrLawParams<ScalarT,FluidSystem>;
    using ThcLawParams = EclThcLawParams<ScalarT>;
    using NullParams = NullThermalConductionLawParams<ScalarT>;

    void setThermalConductionApproach(EclThermalConductionApproach newApproach)
    {
        thermalConductionApproach_ = newApproach;
        switch (thermalConductionApproach()) {
        case EclThermalConductionApproach::Thconr:
            realParams_ = ThconrLawParams{};
            break;

        case EclThermalConductionApproach::Thc:
            realParams_ = ThcLawParams{};
            break;

        case EclThermalConductionApproach::Null:
            realParams_ = NullParams{};
            break;

        case EclThermalConductionApproach::Undefined:
            throw std::logic_error("Cannot set the approach for thermal conduction to 'undefined'!");
        }
    }

    EclThermalConductionApproach thermalConductionApproach() const
    { return thermalConductionApproach_; }

    template<class Function>
    void visit1(Function f)
    {
        std::visit(VisitorOverloadSet{f, [](auto&){}}, realParams_);
    }

    template<class VisitorSet>
    void visit(VisitorSet f) const
    {
        std::visit(f, realParams_);
    }

private:
    EclThermalConductionApproach thermalConductionApproach_ = EclThermalConductionApproach::Undefined;
    std::variant<std::monostate,
                 ThconrLawParams,
                 ThcLawParams,
                 NullParams> realParams_;
};

} // namespace Opm

#endif
