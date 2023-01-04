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

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/common/EnsureFinalized.hpp>

#include <opm/material/thermal/EclHeatcrLawParams.hpp>
#include <opm/material/thermal/EclSpecrockLawParams.hpp>
#include <opm/material/thermal/NullSolidEnergyLawParams.hpp>

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
template <class ScalarT, class FluidSystem>
class EclSolidEnergyLawMultiplexerParams : public EnsureFinalized
{
public:
    using Scalar = ScalarT;

    using HeatcrLawParams = EclHeatcrLawParams<ScalarT,FluidSystem>;
    using SpecrockLawParams = EclSpecrockLawParams<ScalarT>;
    using NullParams = NullSolidEnergyLawParams<ScalarT>;

    void setSolidEnergyApproach(EclSolidEnergyApproach newApproach)
    {
        solidEnergyApproach_ = newApproach;
        switch (solidEnergyApproach()) {
        case EclSolidEnergyApproach::Heatcr:
            realParams_ = HeatcrLawParams{};
            break;

        case EclSolidEnergyApproach::Specrock:
            realParams_ = SpecrockLawParams{};
            break;

        case EclSolidEnergyApproach::Null:
            realParams_ = NullParams{};
            break;
        case EclSolidEnergyApproach::Undefined:
            throw std::runtime_error("Undefined solid energy approach.");
        }
    }

    EclSolidEnergyApproach solidEnergyApproach() const
    { return solidEnergyApproach_; }

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
    EclSolidEnergyApproach solidEnergyApproach_ = EclSolidEnergyApproach::Undefined;
    std::variant<std::monostate,
                 HeatcrLawParams,
                 SpecrockLawParams,
                 NullParams> realParams_;
};

} // namespace Opm

#endif
