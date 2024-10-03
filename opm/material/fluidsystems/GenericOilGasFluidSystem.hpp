// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2023 SINTEF Digital, Mathematics and Cybernetics.

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

#ifndef OPM_GENERICOILGASFLUIDSYSTEM_HPP
#define OPM_GENERICOILGASFLUIDSYSTEM_HPP

#include <opm/common/OpmLog/OpmLog.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#endif

#include <opm/material/eos/PengRobinsonMixture.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp> // TODO: this is something else need to check
#include <opm/material/viscositymodels/LBC.hpp>

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>

#include <fmt/format.h>

namespace Opm {

/*!
 * \ingroup FluidSystem
 *
 * \brief A two phase system that can contain NumComp components
 *
 * \tparam Scalar  The floating-point type that specifies the precision of the numerical operations.
 * \tparam NumComp The number of the components in the fluid system.
 */
    template<class Scalar, int NumComp>
    class GenericOilGasFluidSystem : public BaseFluidSystem<Scalar, GenericOilGasFluidSystem<Scalar, NumComp> > {
    public:
        // TODO: I do not think these should be constant in fluidsystem, will try to make it non-constant later
        static constexpr int numPhases = 2;
        static constexpr int numComponents = NumComp;
        static constexpr int numMisciblePhases = 2;
        // \Note: not totally sure when we should distinguish numMiscibleComponents and numComponents.
        // Possibly when with a dummy phase like water?
        static constexpr int numMiscibleComponents = NumComp;
        // TODO: phase location should be more general
        static constexpr int waterPhaseIdx = -1;
        static constexpr int oilPhaseIdx = 0;
        static constexpr int gasPhaseIdx = 1;

        static constexpr int waterCompIdx = -1;
        static constexpr int oilCompIdx = 0;
        static constexpr int gasCompIdx = 1;
        static constexpr int compositionSwitchIdx = -1; // equil initializer

        template <class ValueType>
        using ParameterCache = Opm::PTFlashParameterCache<ValueType, GenericOilGasFluidSystem<Scalar, NumComp>>;
        using ViscosityModel = Opm::ViscosityModels<Scalar, GenericOilGasFluidSystem<Scalar, NumComp>>;
        using PengRobinsonMixture = Opm::PengRobinsonMixture<Scalar, GenericOilGasFluidSystem<Scalar, NumComp>>;

        struct ComponentParam {
            std::string name;
            Scalar molar_mass; // unit: g/mol
            Scalar critic_temp; // unit: K
            Scalar critic_pres; // unit: parscal
            Scalar critic_vol; // unit: m^3/kmol
            Scalar acentric_factor; // unit: dimension less

            ComponentParam(const std::string_view name_, const Scalar molar_mass_, const Scalar critic_temp_,
                           const Scalar critic_pres_, const Scalar critic_vol_, const Scalar acentric_factor_)
                    : name(name_),
                      molar_mass(molar_mass_),
                      critic_temp(critic_temp_),
                      critic_pres(critic_pres_),
                      critic_vol(critic_vol_),
                      acentric_factor(acentric_factor_)
            {}
        };

        static bool phaseIsActive(unsigned phaseIdx)
        {
            return phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx;
        }

        template<typename Param>
        static void addComponent(const Param& param)
        {
            // Check if the current size is less than the maximum allowed components.
            if (component_param_.size() < numComponents) {
                component_param_.push_back(param);
            } else {
                // Adding another component would exceed the limit.
                const std::string msg = fmt::format("The fluid system has reached its maximum capacity of {} components,"
                                                    "the component '{}' will not be added.", NumComp, param.name);
                OpmLog::note(msg);
                // Optionally, throw an exception?
            }
        }

#if HAVE_ECL_INPUT
        /*!
         * \brief Initialize the fluid system using an ECL deck object
         */
        static void initFromState(const EclipseState& eclState, const Schedule& /* schedule */)
        {
            // TODO: we are not considering the EOS region for now
            const auto& comp_config = eclState.compositionalConfig();
            // how should we utilize the numComps from the CompositionalConfig?
            using FluidSystem = GenericOilGasFluidSystem<Scalar, NumComp>;
            const std::size_t num_comps = comp_config.numComps();
            // const std::size_t num_eos_region = comp_config.
            assert(num_comps == NumComp);
            const auto& names = comp_config.compName();
            // const auto& eos_type = comp_config.eosType(0);
            const auto& molar_weight = comp_config.molecularWeights(0);
            const auto& acentric_factor = comp_config.acentricFactors(0);
            const auto& critic_pressure = comp_config.criticalPressure(0);
            const auto& critic_temp = comp_config.criticalTemperature(0);
            const auto& critic_volume = comp_config.criticalVolume(0);
            FluidSystem::init();
            using CompParm = typename FluidSystem::ComponentParam;
            for (std::size_t c = 0; c < num_comps; ++c) {
                // we use m^3/kmol for the critic volume in the flash calculation, so we multiply 1.e3 for the critic volume
                FluidSystem::addComponent(CompParm{names[c], molar_weight[c], critic_temp[c], critic_pressure[c],
                                                   critic_volume[c] * 1.e3, acentric_factor[c]});
            }
            FluidSystem::printComponentParams();
            interaction_coefficients_ = comp_config.binaryInteractionCoefficient(0);
        }
#endif // HAVE_ECL_INPUT

        static void init()
        {
            component_param_.reserve(numComponents);
        }

        /*!
         * \brief The acentric factor of a component [].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar acentricFactor(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return component_param_[compIdx].acentric_factor;
        }
        /*!
         * \brief Critical temperature of a component [K].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalTemperature(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return component_param_[compIdx].critic_temp;
        }
        /*!
         * \brief Critical pressure of a component [Pa].
         *
         * \copydetails Doxygen::compIdxParam
         */
        static Scalar criticalPressure(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return component_param_[compIdx].critic_pres;
        }
        /*!
        * \brief Critical volume of a component [m3].
        *
        * \copydetails Doxygen::compIdxParam
        */
        static Scalar criticalVolume(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return component_param_[compIdx].critic_vol;
        }

        //! \copydoc BaseFluidSystem::molarMass
        static Scalar molarMass(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return component_param_[compIdx].molar_mass;
        }

        /*!
         * \brief Returns the interaction coefficient for two components.
         *.
         */
        static Scalar interactionCoefficient(unsigned comp1Idx, unsigned comp2Idx)
        {
            assert(isConsistent());
            assert(comp1Idx < numComponents);
            assert(comp2Idx < numComponents);
            if (interaction_coefficients_.empty() || comp2Idx == comp1Idx) {
                return 0.0;
            }
            // make sure row is the bigger value compared to column number
            const auto [column, row] = std::minmax(comp1Idx, comp2Idx);
            const unsigned index = (row * (row - 1) / 2 + column); // it is the current understanding
            return interaction_coefficients_[index];
        }

        //! \copydoc BaseFluidSystem::phaseName
        static std::string_view phaseName(unsigned phaseIdx)
        {
                static const std::string_view name[] = {"o",  // oleic phase
                                                        "g"};  // gas phase

                assert(phaseIdx < numPhases);
                return name[phaseIdx];
        }

        //! \copydoc BaseFluidSystem::componentName
        static std::string_view componentName(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return component_param_[compIdx].name;
        }

        /*!
         * \copydoc BaseFluidSystem::density
         */
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval density(const FluidState& fluidState,
                               const ParameterCache<ParamCacheEval>& paramCache,
                               unsigned phaseIdx)
        {
            assert(isConsistent());
            assert(phaseIdx < numPhases);

            if (phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx) {
                return decay<LhsEval>(fluidState.averageMolarMass(phaseIdx) / paramCache.molarVolume(phaseIdx));
            }

            return {};
        }

        //! \copydoc BaseFluidSystem::viscosity
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval viscosity(const FluidState& fluidState,
                                 const ParameterCache<ParamCacheEval>& paramCache,
                                 unsigned phaseIdx)
        {
            assert(isConsistent());
            assert(phaseIdx < numPhases);
            // Use LBC method to calculate viscosity
            return decay<LhsEval>(ViscosityModel::LBC(fluidState, paramCache, phaseIdx));
        }

        //! \copydoc BaseFluidSystem::fugacityCoefficient
        template <class FluidState, class LhsEval = typename FluidState::Scalar, class ParamCacheEval = LhsEval>
        static LhsEval fugacityCoefficient(const FluidState& fluidState,
                                           const ParameterCache<ParamCacheEval>& paramCache,
                                           unsigned phaseIdx,
                                           unsigned compIdx)
        {
            assert(isConsistent());
            assert(phaseIdx < numPhases);
            assert(compIdx < numComponents);

            return decay<LhsEval>(PengRobinsonMixture::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx));
        }

        // TODO: the following interfaces are needed by function checkFluidSystem()
        //! \copydoc BaseFluidSystem::isCompressible
        static bool isCompressible([[maybe_unused]] unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return true;
        }

        //! \copydoc BaseFluidSystem::isIdealMixture
        static bool isIdealMixture([[maybe_unused]] unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return false;
        }

        //! \copydoc BaseFluidSystem::isLiquid
        static bool isLiquid(unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return (phaseIdx == 0);
        }

        //! \copydoc BaseFluidSystem::isIdealGas
        static bool isIdealGas(unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return (phaseIdx == 1);
        }

    private:
        static bool isConsistent() {
            return component_param_.size() == NumComp;
        }

        static std::vector<ComponentParam> component_param_;
        static std::vector<Scalar> interaction_coefficients_;

    public:
        static std::string printComponentParams() {
            std::string result = "Components Information:\n";
            for (const auto& param : component_param_) {
                result += fmt::format("Name: {}\n", param.name);
                result += fmt::format("Molar Mass: {} g/mol\n", param.molar_mass);
                result += fmt::format("Critical Temperature: {} K\n", param.critic_temp);
                result += fmt::format("Critical Pressure: {} Pascal\n", param.critic_pres);
                result += fmt::format("Critical Volume: {} m^3/kmol\n", param.critic_vol);
                result += fmt::format("Acentric Factor: {}\n", param.acentric_factor);
                result += "---------------------------------\n";
            }
            return result;
        }
    };

    template <class Scalar, int NumComp>
    std::vector<typename GenericOilGasFluidSystem<Scalar, NumComp>::ComponentParam>
    GenericOilGasFluidSystem<Scalar, NumComp>::component_param_;

    template <class Scalar, int NumComp>
    std::vector<Scalar>
    GenericOilGasFluidSystem<Scalar, NumComp>::interaction_coefficients_;
}
#endif // OPM_GENERICOILGASFLUIDSYSTEM_HPP
