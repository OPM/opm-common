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
#ifndef CUBIC_EOS_PARAMS_HPP
#define CUBIC_EOS_PARAMS_HPP

#include <opm/material/Constants.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <opm/material/eos/PRParams.hpp>
#include <opm/material/eos/RKParams.hpp>
#include <opm/material/eos/SRKParams.hpp>

namespace Opm
{

template <class Scalar, class FluidSystem, unsigned phaseIdx>
class CubicEOSParams
{
    enum { numComponents = FluidSystem::numComponents };
    
    static constexpr Scalar R = Constants<Scalar>::R;

    using EOSType = CompositionalConfig::EOSType;
    using PR = Opm::PRParams<Scalar, FluidSystem>;
    using RK = Opm::RKParams<Scalar, FluidSystem>;
    using SRK = Opm::SRKParams<Scalar, FluidSystem>;

public:

    void setEOSType(const EOSType eos_type)
    {
        EosType_= eos_type;
    }

    void updatePure(Scalar temperature, Scalar pressure)
    {
        Valgrind::CheckDefined(temperature);
        Valgrind::CheckDefined(pressure);

        // calculate Ai and Bi
        for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
            Scalar pr = pressure / FluidSystem::criticalPressure(compIdx);
            Scalar Tr = temperature / FluidSystem::criticalTemperature(compIdx);
            Scalar OmegaA = OmegaA_(temperature, compIdx);
            Scalar OmegaB = OmegaB_();

            Scalar newA = OmegaA * pr / (Tr * Tr);
            Scalar newB = OmegaB * pr / Tr;
            assert(std::isfinite(scalarValue(newA)));
            assert(std::isfinite(scalarValue(newB)));

            setAi(newA, compIdx);
            setBi(newB, compIdx);
            Valgrind::CheckDefined(Ai(compIdx));
            Valgrind::CheckDefined(Bi(compIdx));
        }

        // Update Aij
        updateACache_();

    }
    
    template <class FluidState>
    void updateMix(const FluidState& fs)
    {
        using FlashEval = typename FluidState::Scalar;
        
        FlashEval newA = 0;
        FlashEval newB = 0;
        for (unsigned compIIdx = 0; compIIdx < numComponents; ++compIIdx) {
            const FlashEval moleFracI = fs.moleFraction(phaseIdx, compIIdx);
            FlashEval xi = max(0.0, min(1.0, moleFracI));
            Valgrind::CheckDefined(xi);

            for (unsigned compJIdx = 0; compJIdx < numComponents; ++compJIdx) {
                const FlashEval moleFracJ = fs.moleFraction(phaseIdx, compJIdx );
                FlashEval xj = max(0.0, min(1.0, moleFracJ));
                Valgrind::CheckDefined(xj);

                // Calculate A 
                newA +=  xi * xj * aCache_[compIIdx][compJIdx];
                assert(std::isfinite(scalarValue(newA)));
            }

            // Calculate B
            newB += max(0.0, xi) * Bi(compIIdx);
            assert(std::isfinite(scalarValue(newB)));
        }

        // assign A and B
        setA(decay<Scalar>(newA));
        setB(decay<Scalar>(newB));
        Valgrind::CheckDefined(A());
        Valgrind::CheckDefined(B());
    }

    template <class FluidState>
    void updateSingleMoleFraction(const FluidState& fs,
                                  unsigned /*compIdx*/)
    {
        updateMix(fs);
    }

    Scalar aCache(unsigned compIIdx, unsigned compJIdx ) const
    {
        return aCache_[compIIdx][compJIdx];
    }

    void setAi(Scalar value, unsigned compIdx)
    { 
        Ai_[compIdx] = value; 
    }

    void setBi(Scalar value, unsigned compIdx)
    { 
        Bi_[compIdx] = value; 
    }

    Scalar Ai(unsigned compIdx) const
    {
        return Ai_[compIdx];
    }

    Scalar Bi(unsigned compIdx) const
    {
        return Bi_[compIdx];
    }

    void setA(Scalar value)
    { 
        A_ = value; 
    }

    void setB(Scalar value)
    { 
        B_ = value; 
    }

    Scalar A() const
    {
        return A_;
    }

    Scalar B() const
    {
        return B_;
    }

    Scalar m1() const
    {
        switch (EosType_) {
            case EOSType::PRCORR:
            case EOSType::PR: 
                return PR::calcm1();
            case EOSType::RK: 
                return RK::calcm1();
            case EOSType::SRK: 
                return SRK::calcm1();
            default: 
                throw std::runtime_error("EOS type not implemented!");
        }
    }   
    
    Scalar m2() const
    {
        switch (EosType_) {
            case EOSType::PRCORR:
            case EOSType::PR:
                return PR::calcm2();
            case EOSType::RK:
                return RK::calcm2();
            case EOSType::SRK:
                return SRK::calcm2();
            default: 
                throw std::runtime_error("EOS type not implemented!");
        }
    }

protected:
    std::array<Scalar, numComponents> Ai_;
    std::array<Scalar, numComponents> Bi_;
    Scalar A_;
    Scalar B_;
    std::array<std::array<Scalar, numComponents>, numComponents> aCache_;

    EOSType EosType_;

private:
    void updateACache_()
    {
        for (unsigned compIIdx = 0; compIIdx < numComponents; ++ compIIdx) {
            for (unsigned compJIdx = 0; compJIdx < numComponents; ++ compJIdx) {
                // interaction coefficient as given in SPE5
                Scalar Psi = FluidSystem::interactionCoefficient(compIIdx, compJIdx);

                aCache_[compIIdx][compJIdx] = sqrt(Ai(compIIdx) * Ai(compJIdx)) * (1 - Psi);
            }
        }
    }

    Scalar OmegaA_(Scalar temperature, unsigned compIdx)
    {
        switch (EosType_) {
            case EOSType::PRCORR:
                return PR::calcOmegaA(temperature, compIdx, /*modified=*/true);
            case EOSType::PR:
                return PR::calcOmegaA(temperature, compIdx, /*modified=*/false);
            case EOSType::RK:
                return RK::calcOmegaA(temperature, compIdx);
            case EOSType::SRK:
                return SRK::calcOmegaA(temperature, compIdx);
            default: 
                throw std::runtime_error("EOS type not implemented!");
        }
    }

    Scalar OmegaB_()
    {
        switch (EosType_) {
            case EOSType::PRCORR:
            case EOSType::PR:
                return PR::calcOmegaB();
            case EOSType::RK:
                return RK::calcOmegaB();
            case EOSType::SRK:
                return SRK::calcOmegaB();
            default: 
                throw std::runtime_error("EOS type not implemented!");
        }
    }

};  // class CubicEOSParams

}  // namespace Opm

#endif
