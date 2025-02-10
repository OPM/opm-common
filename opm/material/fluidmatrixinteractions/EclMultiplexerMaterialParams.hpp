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

#include "EclStone1Material.hpp"
#include "EclStone2Material.hpp"
#include "EclDefaultMaterial.hpp"
#include "EclTwoPhaseMaterial.hpp"

#include <cassert>
#include <memory>
#include <type_traits>

#include <opm/material/common/EnsureFinalized.hpp>

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
    using EnsureFinalized :: finalize;

    /*!
     * \brief The multiplexer constructor.
     */
    EclMultiplexerMaterialParams() : realParams_()
    {
    }

    EclMultiplexerMaterialParams(const EclMultiplexerMaterialParams& other)
        : realParams_()
    {
        setApproach( other.approach() );
    }

    EclMultiplexerMaterialParams& operator= ( const EclMultiplexerMaterialParams& other )
    {
        realParams_.reset();
        setApproach( other.approach() );
        return *this;
    }

    void setApproach(EclMultiplexerApproach newApproach)
    {
        assert(realParams_ == 0);
        approach_ = newApproach;

        switch (approach()) {
        case EclMultiplexerApproach::Stone1:
            realParams_ = ParamPointerType(new Stone1Params, Deleter< Stone1Params > () );
            break;

        case EclMultiplexerApproach::Stone2:
            realParams_ = ParamPointerType(new Stone2Params, Deleter< Stone2Params > () );
            break;

        case EclMultiplexerApproach::Default:
            realParams_ = ParamPointerType(new DefaultParams, Deleter< DefaultParams > () );
            break;

        case EclMultiplexerApproach::TwoPhase:
            realParams_ = ParamPointerType(new TwoPhaseParams, Deleter< TwoPhaseParams > () );
            break;

        case EclMultiplexerApproach::OnePhase:
            // Do nothing, no parameters.
            break;
        }
    }

    EclMultiplexerApproach approach() const
    { return approach_; }

    // get the parameter object for the Stone1 case
    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::Stone1, Stone1Params>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<Stone1Params>();
    }

    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::Stone1, const Stone1Params>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<Stone1Params>();
    }

    // get the parameter object for the Stone2 case
    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::Stone2, Stone2Params>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<Stone2Params>();
    }

    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::Stone2, const Stone2Params>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<Stone2Params>();
    }

    // get the parameter object for the default case
    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::Default, DefaultParams>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<DefaultParams>();
    }

    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::Default, const DefaultParams>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<DefaultParams>();
    }

    // get the parameter object for the twophase case
    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::TwoPhase, TwoPhaseParams>::type&
    getRealParams()
    {
        assert(approach() == approachV);
        return this->template castTo<TwoPhaseParams>();
    }

    template <EclMultiplexerApproach approachV>
    typename std::enable_if<approachV == EclMultiplexerApproach::TwoPhase, const TwoPhaseParams>::type&
    getRealParams() const
    {
        assert(approach() == approachV);
        return this->template castTo<TwoPhaseParams>();
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        switch (approach()) {
        case EclMultiplexerApproach::Stone1:
            serializer(castTo<Stone1Params>());
            break;

        case EclMultiplexerApproach::Stone2:
            serializer(castTo<Stone2Params>());
            break;

        case EclMultiplexerApproach::Default:
            serializer(castTo<DefaultParams>());
            break;

        case EclMultiplexerApproach::TwoPhase:
            serializer(castTo<TwoPhaseParams>());
            break;

        case EclMultiplexerApproach::OnePhase:
            // Do nothing, no parameters.
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

    EclMultiplexerApproach approach_{EclMultiplexerApproach::Default};
    ParamPointerType realParams_;
};
} // namespace Opm

#endif
