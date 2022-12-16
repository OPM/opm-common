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
 * \copydoc Opm::EclMultiplexerMaterialParams
 */
#ifndef OPM_ECL_MULTIPLEXER_MATERIAL_PARAMS_HPP
#define OPM_ECL_MULTIPLEXER_MATERIAL_PARAMS_HPP

#include <opm/material/common/EnsureFinalized.hpp>

#include <opm/material/fluidmatrixinteractions/EclStone1Material.hpp>
#include <opm/material/fluidmatrixinteractions/EclStone2Material.hpp>
#include <opm/material/fluidmatrixinteractions/EclDefaultMaterial.hpp>
#include <opm/material/fluidmatrixinteractions/EclTwoPhaseMaterial.hpp>

#include <variant>

namespace Opm {

enum class EclMultiplexerApproach {
    Default,
    Stone1,
    Stone2,
    TwoPhase,
    OnePhase
};

/*!
 * \brief Multiplexer implementation for the parameters required by the
 *        multiplexed three-phase material law.
 *
 * Essentially, this class just stores parameter object for the "nested" material law and
 * provides some methods to convert to it.
 */
template<class Traits, class GasOilMaterialLawT, class OilWaterMaterialLawT, class GasWaterMaterialLawT>
class EclMultiplexerMaterialParams : public Traits, public EnsureFinalized
{
    using Scalar = typename Traits::Scalar;
    enum { numPhases = 3 };

    using Stone1Material = EclStone1Material<Traits, GasOilMaterialLawT, OilWaterMaterialLawT>;
    using Stone2Material = EclStone2Material<Traits, GasOilMaterialLawT, OilWaterMaterialLawT>;
    using DefaultMaterial = EclDefaultMaterial<Traits, GasOilMaterialLawT, OilWaterMaterialLawT>;
    using TwoPhaseMaterial = EclTwoPhaseMaterial<Traits, GasOilMaterialLawT, OilWaterMaterialLawT, GasWaterMaterialLawT>;

    using Stone1Params = typename Stone1Material::Params;
    using Stone2Params = typename Stone2Material::Params;
    using DefaultParams = typename DefaultMaterial::Params;
    using TwoPhaseParams = typename TwoPhaseMaterial::Params;

public:
    using EnsureFinalized :: finalize;

    void setApproach(EclMultiplexerApproach newApproach)
    {
        approach_ = newApproach;

        switch (approach()) {
        case EclMultiplexerApproach::Stone1:
            realParams_ = Stone1Params{};
            break;

        case EclMultiplexerApproach::Stone2:
            realParams_ = Stone2Params{};
            break;

        case EclMultiplexerApproach::Default:
            realParams_ = DefaultParams{};
            break;

        case EclMultiplexerApproach::TwoPhase:
            realParams_ = TwoPhaseParams{};
            break;

        case EclMultiplexerApproach::OnePhase:
            // Do nothing, no parameters.
            break;
        }
    }

    EclMultiplexerApproach approach() const
    { return approach_; }

    //! \brief Mutable visit for all types using a visitor set.
    template<class VisitorSet>
    void visit(VisitorSet f)
    {
        std::visit(f, realParams_);
    }

    //! \brief Immutable visit for all types using a visitor set.
    template<class Function>
    void visit(Function f) const
    {
        std::visit(f, realParams_);
    }

private:
    EclMultiplexerApproach approach_ = EclMultiplexerApproach::OnePhase;
    std::variant<std::monostate, // One phase
                 DefaultParams,
                 Stone1Params,
                 Stone2Params,
                 TwoPhaseParams> realParams_;
};

} // namespace Opm

#endif
