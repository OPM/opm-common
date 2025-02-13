// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 NORCE.
  Copyright 2022 SINTEF Digital, Mathematics and Cybernetics.

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
 * \copydoc Opm::PTFlashParameterCache
 */
#ifndef OPM_PTFlash_PARAMETER_CACHE_HPP
#define OPM_PTFlash_PARAMETER_CACHE_HPP

#include <opm/material/common/Valgrind.hpp>
#include <opm/material/fluidsystems/ParameterCacheBase.hpp>
#include <opm/material/eos/CubicEOS.hpp>
#include <opm/material/eos/CubicEOSParams.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>

#include <cassert>

namespace Opm {

/*!
 * \ingroup Fluidsystems
 * \brief Specifies the parameter cache used by the SPE-5 fluid system.
 */
template <class Scalar, class FluidSystem>
class PTFlashParameterCache
    : public Opm::ParameterCacheBase<PTFlashParameterCache<Scalar, FluidSystem> >
{
    using ThisType = PTFlashParameterCache<Scalar, FluidSystem>;
    using ParentType = Opm::ParameterCacheBase<ThisType>;
    using CubicEOS = Opm::CubicEOS<Scalar, FluidSystem>;
    using EOSType = CompositionalConfig::EOSType;

    enum { numPhases = FluidSystem::numPhases };
    enum { oilPhaseIdx = FluidSystem::oilPhaseIdx };
    enum { gasPhaseIdx = FluidSystem::gasPhaseIdx };
    enum { waterPhaseIdx = FluidSystem::waterPhaseIdx };

    static constexpr bool waterEnabled = FluidSystem::waterEnabled;

    static_assert(static_cast<int>(oilPhaseIdx) >= 0, "Oil phase index must be non-negative");
    static_assert(static_cast<int>(oilPhaseIdx) < static_cast<int>(numPhases),
                  "Oil phase index must be strictly less than FluidSystem's number of phases");

    static_assert(static_cast<int>(gasPhaseIdx) >= 0, "Gas phase index must be non-negative");
    static_assert(static_cast<int>(gasPhaseIdx) < static_cast<int>(numPhases),
                  "Gas phase index must be strictly less than FluidSystem's number of phases");

public:
    //! The cached parameters for the oil phase
    using OilPhaseParams = Opm::CubicEOSParams<Scalar, FluidSystem, oilPhaseIdx>;

    //! The cached parameters for the gas phase
    using GasPhaseParams = Opm::CubicEOSParams<Scalar, FluidSystem, gasPhaseIdx>;

    explicit PTFlashParameterCache(EOSType eos_type)
    {
        VmUpToDate_[oilPhaseIdx] = false;
        Valgrind::SetUndefined(Vm_[oilPhaseIdx]);
        VmUpToDate_[gasPhaseIdx] = false;
        Valgrind::SetUndefined(Vm_[gasPhaseIdx]);
    
        oilPhaseParams_.setEOSType(eos_type);
        gasPhaseParams_.setEOSType(eos_type);
    }

    //! \copydoc ParameterCacheBase::updatePhase
    template <class FluidState>
    void updatePhase(const FluidState& fluidState,
                     unsigned phaseIdx,
                     int exceptQuantities = ParentType::None)
    {
        if (waterEnabled && phaseIdx == static_cast<unsigned int>(waterPhaseIdx)) {
            return;
        }

        assert ((phaseIdx == static_cast<unsigned int>(oilPhaseIdx)) ||
                (phaseIdx == static_cast<unsigned int>(gasPhaseIdx)));

        updateEosParams(fluidState, phaseIdx, exceptQuantities);

        // update the phase's molar volume
        updateMolarVolume_(fluidState, phaseIdx);
    }

    //! \copydoc ParameterCacheBase::updateSingleMoleFraction
    template <class FluidState>
    void updateSingleMoleFraction(const FluidState& fluidState,
                                  unsigned phaseIdx,
                                  unsigned compIdx)
    {
        if (phaseIdx == oilPhaseIdx)
            oilPhaseParams_.updateSingleMoleFraction(fluidState, compIdx);
        if (phaseIdx == gasPhaseIdx)
            gasPhaseParams_.updateSingleMoleFraction(fluidState, compIdx);
        else
            return;

        // update the phase's molar volume
        updateMolarVolume_(fluidState, phaseIdx);
    }

    Scalar A(unsigned phaseIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.A();
        case gasPhaseIdx: return gasPhaseParams_.A();
        default:
            throw std::logic_error("The A parameter is only defined for "
                                   "oil and gas phases");
        };
    }
    
    Scalar B(unsigned phaseIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.B();
        case gasPhaseIdx: return gasPhaseParams_.B();
        default:
            throw std::logic_error("The B parameter is only defined for "
                                   "oil and gas phases");
        };
    }
    
    Scalar Bi(unsigned phaseIdx, unsigned compIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.Bi(compIdx);
        case gasPhaseIdx: return gasPhaseParams_.Bi(compIdx);
        default:
            throw std::logic_error("The Bi parameter is only defined for "
                                   "oil and gas phases");
        };
    }

    Scalar m1(unsigned phaseIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.m1();
        case gasPhaseIdx: return gasPhaseParams_.m1();
        default:
            throw std::logic_error("The m1 parameter is only defined for "
                                   "oil and gas phases");
        };
    }

    Scalar m2(unsigned phaseIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.m2();
        case gasPhaseIdx: return gasPhaseParams_.m2();
        default:
            throw std::logic_error("The m2 parameter is only defined for "
                                   "oil and gas phases");
        };
    }

    /*!
     * \brief The Peng-Robinson attractive parameter for a phase.
     *
     * \param phaseIdx The fluid phase of interest
     */
    Scalar a(unsigned phaseIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.a();
        case gasPhaseIdx: return gasPhaseParams_.a();
        default:
            throw std::logic_error("The a() parameter is only defined for "
                                   "oil and gas phases");
        };
    }

    /*!
     * \brief The Peng-Robinson covolume for a phase.
     *
     * \param phaseIdx The fluid phase of interest
     */
    Scalar b(unsigned phaseIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.b();
        case gasPhaseIdx: return gasPhaseParams_.b();
        default:
            throw std::logic_error("The b() parameter is only defined for "
                                   "oil and gas phase");
        };
    }

    /*!
     * \brief The Peng-Robinson attractive parameter for a pure
     *        component given the same temperature and pressure of the
     *        phase.
     *
     * \param phaseIdx The fluid phase of interest
     * \param compIdx The component phase of interest
     */
    Scalar aPure(unsigned phaseIdx, unsigned compIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.pureParams(compIdx).a();
        case gasPhaseIdx: return gasPhaseParams_.pureParams(compIdx).a();
        default:
            throw std::logic_error("The a() parameter is only defined for "
                                   "oil and gas phase");
        };
    }

    /*!
     * \brief The Peng-Robinson covolume for a pure component given
     *        the same temperature and pressure of the phase.
     *
     * \param phaseIdx The fluid phase of interest
     * \param compIdx The component phase of interest
     */
    Scalar bPure(unsigned phaseIdx, unsigned compIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.pureParams(compIdx).b();
        case gasPhaseIdx: return gasPhaseParams_.pureParams(compIdx).b();
        default:
            throw std::logic_error("The b() parameter is only defined for "
                                   "oil and gas phase");
        };
    }

    /*!
     * \brief TODO
     *
     * \param phaseIdx The fluid phase of interest
     * \param compIdx The component phase of interest
     * \param compJIdx Additional component index
     */
    Scalar aCache(unsigned phaseIdx, unsigned compIdx, unsigned compJIdx) const
    {
        switch (phaseIdx)
        {
        case oilPhaseIdx: return oilPhaseParams_.aCache(compIdx, compJIdx);
        case gasPhaseIdx: return gasPhaseParams_.aCache(compIdx, compJIdx);
        default:
            throw std::logic_error("The aCache parameter is only defined for "
                                   "oil and gas phase");
        };
    }

    /*!
     * \brief Returns the molar volume of a phase [m^3/mol]
     *
     * \param phaseIdx The fluid phase of interest
     */
    Scalar molarVolume(unsigned phaseIdx) const
    { 
        assert(VmUpToDate_[phaseIdx]); 
        return Vm_[phaseIdx]; 
    }


    /*!
     * \brief Returns the Peng-Robinson mixture parameters for the oil
     *        phase.
     */
    const OilPhaseParams& oilPhaseParams() const
    { return oilPhaseParams_; }

    /*!
     * \brief Returns the Peng-Robinson mixture parameters for the gas
     *        phase.
     */
    const GasPhaseParams& gasPhaseParams() const
    // { throw std::invalid_argument("gas phase does not exist");}
    { return gasPhaseParams_; }

    /*!
     * \brief Update all parameters required by the equation of state to
     *        calculate some quantities for the phase.
     *
     * \param fluidState The representation of the thermodynamic system of interest.
     * \param phaseIdx The index of the fluid phase of interest.
     * \param exceptQuantities The quantities of the fluid state that have not changed since the last update.
     */
    template <class FluidState>
    void updateEosParams(const FluidState& fluidState,
                         unsigned phaseIdx,
                         int exceptQuantities = ParentType::None)
    {
        assert ((phaseIdx == static_cast<unsigned int>(oilPhaseIdx)) ||
                (phaseIdx == static_cast<unsigned int>(gasPhaseIdx)));

        if (!(exceptQuantities & ParentType::Temperature))
        {
            updatePure_(fluidState, phaseIdx);
            updateMix_(fluidState, phaseIdx);
            VmUpToDate_[phaseIdx] = false;
        }
        else if (!(exceptQuantities & ParentType::Composition))
        {
            updateMix_(fluidState, phaseIdx);
            VmUpToDate_[phaseIdx] = false;
        }
        else if (!(exceptQuantities & ParentType::Pressure)) {
            VmUpToDate_[phaseIdx] = false;
        }
    }

protected:
    /*!
     * \brief Update all parameters of a phase which only depend on
     *        temperature and/or pressure.
     *
     * This usually means the parameters for the pure components.
     */
    template <class FluidState>
    void updatePure_(const FluidState& fluidState, unsigned phaseIdx)
    {
        Scalar T = decay<Scalar>(fluidState.temperature(phaseIdx));
        Scalar p = decay<Scalar>(fluidState.pressure(phaseIdx));

        switch (phaseIdx)
        {
        case oilPhaseIdx: oilPhaseParams_.updatePure(T, p); break;
        case gasPhaseIdx: gasPhaseParams_.updatePure(T, p); break;
        }
    }

    /*!
     * \brief Update all parameters of a phase which depend on the
     *        fluid composition. It is assumed that updatePure() has
     *        been called before this method.
     *
     * Here, the mixing rule kicks in.
     */
    template <class FluidState>
    void updateMix_(const FluidState& fluidState, unsigned phaseIdx)
    {
        Valgrind::CheckDefined(fluidState.averageMolarMass(phaseIdx));
        switch (phaseIdx)
        {
        case oilPhaseIdx:
            oilPhaseParams_.updateMix(fluidState);
            break;
        case gasPhaseIdx:
            gasPhaseParams_.updateMix(fluidState);
            break;
        }
    }

    template <class FluidState>
    void updateMolarVolume_(const FluidState& fluidState,
                            unsigned phaseIdx)
    {
        VmUpToDate_[phaseIdx] = true;

        // calculate molar volume of the phase (we will need this for the
        // fugacity coefficients and the density anyway)
        switch (phaseIdx) {
        case gasPhaseIdx: {
            // calculate molar volumes for the given composition. although
            // this isn't a Peng-Robinson parameter strictly speaking, the
            // molar volume appears in basically every quantity the fluid
            // system can get queried, so it is okay to calculate it
            // here...
            Vm_[gasPhaseIdx] = decay<Scalar> (
                CubicEOS::computeMolarVolume(fluidState,
                                                 *this,
                                                 phaseIdx,
                                                 /*isGasPhase=*/true) );
            break;
        }
        case oilPhaseIdx: {
            // calculate molar volumes for the given composition. although
            // this isn't a Peng-Robinson parameter strictly speaking, the
            // molar volume appears in basically every quantity the fluid
            // system can get queried, so it is okay to calculate it
            // here...
            Vm_[oilPhaseIdx] = decay<Scalar> (
                CubicEOS::computeMolarVolume(fluidState,
                                                 *this,
                                                 phaseIdx,
                                                 /*isGasPhase=*/false) );

            break;
        }
        };
    }

    bool VmUpToDate_[numPhases];
    Scalar Vm_[numPhases];

    OilPhaseParams oilPhaseParams_;
    GasPhaseParams gasPhaseParams_;
};

} // namespace Opm

#endif
