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

#ifndef OPM_GENERIC_OIL_GAS_WATER_FLUIDSYSTEM_HPP
#define OPM_GENERIC_OIL_GAS_WATER_FLUIDSYSTEM_HPP

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/material/Constants.hpp>
#include <opm/material/eos/CubicEOS.hpp>
#include <opm/material/fluidsystems/blackoilpvt/WaterPvtMultiplexer.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>
#include <opm/material/fluidsystems/PTFlashParameterCache.hpp> // TODO: this is something else need to check
#include <opm/material/viscositymodels/ViscosityModels.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace Opm {

/*!
 * \ingroup FluidSystem
 *
 * \brief The parameters of a component of GenericOilGasWaterFluidSystem.
 */
template <class Scalar>
struct CompositionalComponentParam {
    std::string name;
    Scalar molar_mass; // unit: kg/mol
    Scalar critic_temp; // unit: K
    Scalar critic_pres; // unit: parscal
    Scalar critic_vol; // unit: m^3/kmol
    Scalar acentric_factor; // unit: dimension less
    Scalar volume_shift; // dimensionless SSHIFT coefficient
    // OMEGAA and OMEGAB; unset for the constants of the equation of state
    std::optional<Scalar> omega_a;
    std::optional<Scalar> omega_b;

    CompositionalComponentParam(const std::string_view name_,
                                const Scalar molar_mass_,
                                const Scalar critic_temp_,
                                const Scalar critic_pres_,
                                const Scalar critic_vol_,
                                const Scalar acentric_factor_,
                                const Scalar volume_shift_ = 0.0,
                                const std::optional<Scalar> omega_a_ = std::nullopt,
                                const std::optional<Scalar> omega_b_ = std::nullopt)
        : name(name_)
        , molar_mass(molar_mass_)
        , critic_temp(critic_temp_)
        , critic_pres(critic_pres_)
        , critic_vol(critic_vol_)
        , acentric_factor(acentric_factor_)
        , volume_shift(volume_shift_)
        , omega_a(omega_a_)
        , omega_b(omega_b_)
    {}
};

/*!
 * \ingroup FluidSystem
 *
 * \brief The fluid of one equation-of-state region of a compositional deck,
 *        as GenericOilGasWaterFluidSystem takes it.
 */
template <class Scalar>
struct CompositionalRegionParams {
    std::vector<CompositionalComponentParam<Scalar>> components;
    //! As a packed lower triangle (1,0), (2,0), (2,1), ...; empty without BIC.
    std::vector<Scalar> interaction_coefficients;
    //! The coefficients of the Lorentz-Bray-Clark viscosity correlation.
    //! The same for every region: a deck has one LBCCOEF for all of them.
    std::array<Scalar, 5> lbc_coefficients;
};

/*!
 * \ingroup FluidSystem
 *
 * \brief The component parameters, the binary interaction coefficients and
 *        the LBC viscosity coefficients of an equation-of-state region of a
 *        deck. GenericOilGasWaterFluidSystem::initFromState() takes every region.
 *
 * It does not depend on the number of components of the fluid system, so a
 * program can read a deck before it knows that number.
 *
 * \param config the compositional configuration of the deck
 * \param eos_region the zero-based region, below the number of
 *        equation-of-state regions of the deck (TABDIMS item 9)
 *
 * \throw std::runtime_error if the deck gives neither VCRIT nor ZCRIT
 */
template <class Scalar>
CompositionalRegionParams<Scalar>
compositionalRegionParams(const CompositionalConfig& config, const std::size_t eos_region)
{
    const auto& names = config.compName();
    const auto& eos_props = config.eosProps(eos_region);
    // Absent OMEGAA and OMEGAB entries hold the constants of the EOS type.
    const auto& omega_a = config.omegaA(eos_region);
    const auto& omega_b = config.omegaB(eos_region);

    // Critical volume [m^3/mol] of a component. ZCRIT, when given, takes
    // precedence over VCRIT: Vc = Zc R Tc / Pc.
    const auto critical_volume = [&eos_props](const std::size_t compIdx) {
        if (!eos_props.critical_z_factor.empty()) {
            return eos_props.critical_z_factor[compIdx] * Constants<double>::R
                * eos_props.critical_temperature[compIdx] / eos_props.critical_pressure[compIdx];
        }
        if (eos_props.critical_volume.empty()) {
            throw std::runtime_error("The equation of state needs the critical volumes "
                                     "from VCRIT or the critical Z-factors from ZCRIT.");
        }
        return eos_props.critical_volume[compIdx];
    };

    CompositionalRegionParams<Scalar> params;
    for (std::size_t c = 0; c < config.numComps(); ++c) {
        // we use m^3/kmol for the critic volume in the flash calculation, so we multiply 1.e3
        // for the critic volume
        params.components.emplace_back(names[c],
                                       static_cast<Scalar>(eos_props.molecular_weights[c]),
                                       static_cast<Scalar>(eos_props.critical_temperature[c]),
                                       static_cast<Scalar>(eos_props.critical_pressure[c]),
                                       static_cast<Scalar>(critical_volume(c) * 1.e3),
                                       static_cast<Scalar>(eos_props.acentric_factors[c]),
                                       c < eos_props.volume_shifts.size()
                                           ? static_cast<Scalar>(eos_props.volume_shifts[c])
                                           : Scalar {0},
                                       static_cast<Scalar>(omega_a[c]),
                                       static_cast<Scalar>(omega_b[c]));
    }

    const auto& bic = eos_props.binary_interaction_coefficient;
    params.interaction_coefficients.assign(bic.begin(), bic.end());

    const auto& lbc = config.lbcCoefficients();
    std::ranges::copy(lbc, params.lbc_coefficients.begin());

    return params;
}

/*!
 * \ingroup FluidSystem
 *
 * \brief A two phase system that can contain NumComp components
 *
 * \tparam Scalar  The floating-point type that specifies the precision of the numerical operations.
 * \tparam NumComp The number of the components in the fluid system.
 */
    template<class Scalar, int NumComp, bool enableWater>
    class GenericOilGasWaterFluidSystem
        : public BaseFluidSystem<Scalar, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater> > {
    public:
        // TODO: I do not think these should be constant in fluidsystem, will try to make it non-constant later
        static constexpr bool waterEnabled = enableWater;
        static constexpr int numPhases = enableWater ? 3 : 2;
        static constexpr int numComponents = NumComp;
        static constexpr int numMisciblePhases = 2;
        // \Note: not totally sure when we should distinguish numMiscibleComponents and numComponents.
        // Possibly when with a dummy phase like water?
        static constexpr int numMiscibleComponents = NumComp;
        // TODO: phase location should be more general
        static constexpr int oilPhaseIdx = 0;
        static constexpr int gasPhaseIdx = 1;
        static constexpr int waterPhaseIdx = 2;

        static constexpr int waterCompIdx = -1;
        static constexpr int oilCompIdx = 0;
        static constexpr int gasCompIdx = 1;
        static constexpr int compositionSwitchIdx = -1; // equil initializer

        template <class ValueType>
        using ParameterCache = Opm::PTFlashParameterCache<ValueType, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>>;
        using ViscosityModel = Opm::ViscosityModels<Scalar, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>>;
        using CubicEOS = Opm::CubicEOS<Scalar, GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>>;
        using WaterPvt = WaterPvtMultiplexer<Scalar>;
        using EOSType = CompositionalConfig::EOSType;

        using ComponentParam = CompositionalComponentParam<Scalar>;

        static bool phaseIsActive(unsigned phaseIdx)
        {
            if constexpr (enableWater) {
                assert(phaseIdx < numPhases);
                return true;
            }
            else {
                if (phaseIdx == waterPhaseIdx)
                    return false;
                assert(phaseIdx < numPhases);
                return true;
            }
        }

        template<typename Param>
        static void addComponent(const Param& param)
        {
            auto& components = eos_regions_.front().components;
            // Check if the current size is less than the maximum allowed components.
            if (components.size() < numComponents) {
                components.push_back(param);
            } else {
                // Adding another component would exceed the limit.
                throw std::runtime_error {
                    fmt::format("The fluid system has reached its maximum capacity of {} "
                                "components, the component '{}' cannot be added.",
                                NumComp, param.name)
                };
            }
        }

        /*!
         * \brief Set the binary interaction coefficients of the components,
         *        for a fluid system that is configured without a deck.
         *
         * \param bic the coefficients as a packed lower triangle in the order
         *        (1,0), (2,0), (2,1), (3,0), ..., the order in which
         *        interactionCoefficient() reads them. An empty vector sets all
         *        coefficients to zero.
         *
         * \throw std::invalid_argument if bic is neither empty nor of size
         *        numComponents * (numComponents - 1) / 2
         */
        static void setInteractionCoefficients(std::vector<Scalar> bic)
        {
            const auto expected = static_cast<std::size_t>(numComponents) * (numComponents - 1) / 2;
            if (!bic.empty() && bic.size() != expected) {
                throw std::invalid_argument {
                    fmt::format("Expected {} binary interaction coefficients for {} "
                                "components, or none, got {}.",
                                expected, NumComp, bic.size())
                };
            }
            eos_regions_.front().interaction_coefficients = std::move(bic);
        }

        /*!
         * \brief Set the coefficients of the Lorentz-Bray-Clark viscosity
         *        correlation, for a fluid system that is configured without
         *        a deck. From a deck, they come from the LBCCOEF keyword.
         *        init() restores the defaults.
         */
        static void setLbcCoefficients(const std::array<Scalar, 5>& lbc)
        {
            lbc_coefficients_ = lbc;
        }

        /*!
         * \brief Initialize the fluid system using an ECL deck object
         */
        static void initFromState(const EclipseState& eclState, const Schedule& schedule)
        {
            const auto& comp_config = eclState.compositionalConfig();
            // how should we utilize the numComps from the CompositionalConfig?
            using FluidSystem = GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>;
            const std::size_t num_comps = comp_config.numComps();

            // CompositionalConfig is left empty for input the compositional path
            // cannot handle - CO2STORE and H2STORE runs return before any of it
            // is read - so reading the EOS properties below would index an empty
            // container.
            if (num_comps == 0) {
                throw std::runtime_error {
                    fmt::format("The deck has no equation-of-state description, but "
                                "this run needs a compositional fluid system with {} "
                                "components.  CO2STORE and H2STORE runs do not carry "
                                "one; use flow for those decks.", NumComp)
                };
            }

            if (num_comps != NumComp) {
                throw std::runtime_error {
                    fmt::format("The deck describes {} components, but this fluid "
                                "system was built for {}.", num_comps, NumComp)
                };
            }

            // With several reservoir EOS regions, each keeps its own properties
            // and equation of state. The molecular weights must agree: component
            // masses cross region boundaries unchanged, which would otherwise
            // change their amounts. Every region is read before the static data
            // changes, so a rejected deck leaves the fluid system as it was.
            const auto num_eos_regions = eclState.runspec().tabdims().getNumEosRes();
            const auto& molecular_weights = comp_config.eosProps(0).molecular_weights;
            std::vector<EosRegion> eos_regions;
            eos_regions.reserve(num_eos_regions);
            for (std::size_t region = 0; region < num_eos_regions; ++region) {
                if (comp_config.eosProps(region).molecular_weights != molecular_weights) {
                    throw std::runtime_error {
                        fmt::format("The molecular weights of EOS region {} differ from "
                                    "those of EOS region 1. All reservoir EOS regions "
                                    "must use the same molecular weights.", region + 1)
                    };
                }
                auto params = compositionalRegionParams<Scalar>(comp_config, region);
                auto& eos_region = eos_regions.emplace_back();
                eos_region.components = std::move(params.components);
                eos_region.interaction_coefficients = std::move(params.interaction_coefficients);
                eos_region.eos_type = comp_config.eosType(region);
            }

            FluidSystem::init();
            eos_regions_ = std::move(eos_regions);
            eos_type_from_deck_ = true;
            std::ranges::copy(comp_config.lbcCoefficients(), lbc_coefficients_.begin());

            // Init. water pvt from deck
            waterPvt_->initFromState(eclState, schedule);

        }

        static void init()
        {
            waterPvt_ = std::make_shared<WaterPvt>();
            // Discard the previous configuration so subsequent component
            // registrations replace it.
            eos_regions_.assign(1, EosRegion{});
            eos_regions_.front().components.reserve(numComponents);
            eos_type_from_deck_ = false;
            lbc_coefficients_ = ViscosityModel::defaultLBCCoefficients();
        }

        /*!
        * \brief Set the pressure-volume-saturation (PVT) relations for the water phase.
        */
        static void setWaterPvt(std::shared_ptr<WaterPvt> pvtObj)
        { waterPvt_ = std::move(pvtObj); }

        //! The number of reservoir EOS regions.
        static std::size_t numEosRegions()
        { return eos_regions_.size(); }

        /*!
         * \brief The component properties and equation of state of a
         *        reservoir EOS region.
         */
        class EosRegion
        {
        public:
            //! The equation of state of the region.
            EOSType eosType() const
            { return eos_type; }

            /*!
             * \brief The acentric factor of a component [].
             *
             * \copydetails Doxygen::compIdxParam
             */
            Scalar acentricFactor(unsigned compIdx) const
            { return component_(compIdx).acentric_factor; }

            /*!
             * \brief Dimensionless volume-translation coefficient (SSHIFT).
             *
             * The component volume correction is s_c b_c, where b_c is the
             * equation-of-state covolume.
             *
             * \copydetails Doxygen::compIdxParam
             */
            Scalar volumeShift(unsigned compIdx) const
            { return component_(compIdx).volume_shift; }

            /*!
             * \brief The \f$\Omega_a\f$ of a component from OMEGAA, or nothing
             *        when the constant of the equation of state applies.
             *
             * \copydetails Doxygen::compIdxParam
             */
            std::optional<Scalar> omegaA(unsigned compIdx) const
            { return component_(compIdx).omega_a; }

            /*!
             * \brief The \f$\Omega_b\f$ of a component from OMEGAB, or nothing
             *        when the constant of the equation of state applies.
             *
             * \copydetails Doxygen::compIdxParam
             */
            std::optional<Scalar> omegaB(unsigned compIdx) const
            { return component_(compIdx).omega_b; }

            /*!
             * \brief Critical temperature of a component [K].
             *
             * \copydetails Doxygen::compIdxParam
             */
            Scalar criticalTemperature(unsigned compIdx) const
            { return component_(compIdx).critic_temp; }

            /*!
             * \brief Critical pressure of a component [Pa].
             *
             * \copydetails Doxygen::compIdxParam
             */
            Scalar criticalPressure(unsigned compIdx) const
            { return component_(compIdx).critic_pres; }

            /*!
             * \brief Critical volume of a component [m3].
             *
             * \copydetails Doxygen::compIdxParam
             */
            Scalar criticalVolume(unsigned compIdx) const
            { return component_(compIdx).critic_vol; }

            /*!
             * \brief Returns the interaction coefficient for two components.
             */
            Scalar interactionCoefficient(unsigned comp1Idx, unsigned comp2Idx) const
            {
                assert(isConsistent());
                assert(comp1Idx < numComponents);
                assert(comp2Idx < numComponents);
                if (interaction_coefficients.empty() || comp2Idx == comp1Idx) {
                    return 0.0;
                }
                // make sure row is the bigger value compared to column number
                const auto [column, row] = std::minmax(comp1Idx, comp2Idx);
                const unsigned index = (row * (row - 1) / 2 + column); // it is the current understanding
                return interaction_coefficients[index];
            }

        private:
            friend GenericOilGasWaterFluidSystem;

            const ComponentParam& component_(unsigned compIdx) const
            {
                assert(isConsistent());
                assert(compIdx < numComponents);
                return components[compIdx];
            }

            std::vector<ComponentParam> components;
            std::vector<Scalar> interaction_coefficients;
            EOSType eos_type = EOSType::PR;
        };

        /*!
         * \brief The component properties and equation of state of a
         *        reservoir EOS region.
         *
         * \param eosRegionIdx The reservoir EOS region.
         * \throw std::out_of_range if the fluid system does not have the region
         */
        static const EosRegion& eosRegion(unsigned eosRegionIdx)
        {
            if (eosRegionIdx >= numEosRegions()) {
                throw std::out_of_range {
                    fmt::format("EOS region {} is selected, but the fluid system has {}.",
                                eosRegionIdx + 1, numEosRegions())
                };
            }
            return eos_regions_[eosRegionIdx];
        }

        /*!
         * \brief Set the equation of state of a fluid system set up without a
         *        deck. It is Peng-Robinson unless set here.
         *
         * A deck fills absent OMEGAA and OMEGAB entries with the constants of
         * its equation of state, so a fluid system initialised from a deck
         * keeps the deck's equation of state and rejects the call.
         */
        static void setEosType(const EOSType type)
        {
            if (eos_type_from_deck_) {
                throw std::logic_error("The equation of state of a fluid system "
                                       "initialised from a deck cannot be changed.");
            }
            eos_regions_.front().eos_type = type;
        }

        //! \copydoc BaseFluidSystem::molarMass
        static Scalar molarMass(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return eos_regions_.front().components[compIdx].molar_mass;
        }

        /*!
         * \brief Coefficients for the Lorentz-Bray-Clark viscosity correlation.
         *
         * Defaults to the standard LBC coefficients unless modified
         * with the LBCCOEF keyword.
         */
        static const std::array<Scalar, 5>& lbcCoefficients()
        { return lbc_coefficients_; }

        //! \copydoc BaseFluidSystem::phaseName
        static std::string_view phaseName(unsigned phaseIdx)
        {
                static const std::string_view name[] = {"o",  // oleic phase
                                                        "g",  // gas phase
                                                        "w"};  // aqueous phase

                assert(phaseIdx < numPhases);
                return name[phaseIdx];
        }

        //! \copydoc BaseFluidSystem::componentName
        static std::string_view componentName(unsigned compIdx)
        {
            assert(isConsistent());
            assert(compIdx < numComponents);

            return eos_regions_.front().components[compIdx].name;
        }

        /*!
         * \copydoc BaseFluidSystem::density
         */
        template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
        static LhsEval density(const FluidState& fluidState,
                               const ParameterCache<ParamCacheEval>& paramCache,
                               unsigned phaseIdx)
        {
            assert(isConsistent());
            assert(phaseIdx < numPhases);

            if (phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx) {
                // Density uses the translated volume; fugacity coefficients
                // use the unshifted equation-of-state root.
                const auto Vm = paramCache.correctedMolarVolume(phaseIdx);
                return decay<LhsEval>(fluidState.averageMolarMass(phaseIdx) / Vm);
            }
            else {
                const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
                const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
                const unsigned pvtRegionIdx = paramCache.pvtRegionIndex();
                const Scalar& rho_w_ref = waterPvt_->waterReferenceDensity(pvtRegionIdx);
                const LhsEval& bw = waterPvt_->inverseFormationVolumeFactor(pvtRegionIdx, T, p, LhsEval(0.0), LhsEval(0.0));
                return rho_w_ref * bw;
            }

            return {};
        }

        //! \copydoc BaseFluidSystem::viscosity
        template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
        static LhsEval viscosity(const FluidState& fluidState,
                                 const ParameterCache<ParamCacheEval>& paramCache,
                                 unsigned phaseIdx)
        {
            assert(isConsistent());
            assert(phaseIdx < numPhases);

            if (phaseIdx == oilPhaseIdx || phaseIdx == gasPhaseIdx) {
                // LBC is a reduced-density correlation, so use the physical
                // molar density after applying SSHIFT.
                const auto rho = density(fluidState, paramCache, phaseIdx);
                const auto molarDensity = rho / fluidState.averageMolarMass(phaseIdx);
                return decay<LhsEval>(
                    ViscosityModel::LBCWithMolarDensity(fluidState, molarDensity, phaseIdx,
                                                        paramCache.eosRegion()));
            }
            else {
                const LhsEval& p = decay<LhsEval>(fluidState.pressure(phaseIdx));
                const LhsEval& T = decay<LhsEval>(fluidState.temperature(phaseIdx));
                return waterPvt_->viscosity(paramCache.pvtRegionIndex(), T, p, LhsEval(0.0), LhsEval(0.0));
            }
        }

        /*!
         * \copydoc BaseFluidSystem::fugacityCoefficient
         *
         * The cubic EOS is evaluated on the unshifted root, and the SSHIFT
         * translation enters as the per-component factor exp(-p s_c b_c /
         * (R T)), written here as exp(-s_c B_c) since B_c = b_c p / (R T).
         * The factor is the same in both phases at equal phase pressure and
         * temperature, so it cancels from the equilibrium ratios and leaves
         * the phase split untouched.
         */
        template <class FluidState, class LhsEval = typename FluidState::ValueType, class ParamCacheEval = LhsEval>
        static LhsEval fugacityCoefficient(const FluidState& fluidState,
                                           const ParameterCache<ParamCacheEval>& paramCache,
                                           unsigned phaseIdx,
                                           unsigned compIdx)
        {
            if (waterEnabled && phaseIdx == static_cast<unsigned int>(waterPhaseIdx))
                return LhsEval(0.0);

            assert(isConsistent());
            assert(phaseIdx < numPhases);
            assert(compIdx < numComponents);

            const auto fugCoeff =
                CubicEOS::computeFugacityCoefficient(fluidState, paramCache, phaseIdx, compIdx);
            const auto translation =
                exp(-paramCache.eosRegion().volumeShift(compIdx)
                    * paramCache.Bi(phaseIdx, compIdx));

            return decay<LhsEval>(fugCoeff * translation);
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

            return (phaseIdx == oilPhaseIdx);
        }

        //! \copydoc BaseFluidSystem::isIdealGas
        static bool isIdealGas(unsigned phaseIdx)
        {
            assert(phaseIdx < numPhases);

            return (phaseIdx == gasPhaseIdx);
        }
        // the following funcitons are needed to compile the GenericOutputBlackoilModule
        // not implemented for this FluidSystem yet
        template <class LhsEval>
        static LhsEval convertXwGToxwG(const LhsEval&, unsigned)
        {
            assert(false && "convertXwGToxwG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }
        template <class LhsEval>
        static LhsEval convertXoGToxoG(const LhsEval&, unsigned)
        {
            assert(false && "convertXoGToxoG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertxoGToXoG(const LhsEval&, unsigned)
        {
            assert(false && "convertxoGToXoG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertXgOToxgO(const LhsEval&, unsigned)
        {
            assert(false && "convertXgOToxgO not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertRswToXwG(const LhsEval&, unsigned)
        {
            assert(false && "convertRswToXwG not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertRvwToXgW(const LhsEval&, unsigned)
        {
            assert(false && "convertRvwToXgW not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        template <class LhsEval>
        static LhsEval convertXgWToxgW(const LhsEval&, unsigned)
        {
            assert(false && "convertXgWToxgW not implemented for GenericOilGasWaterFluidSystem!");
            return 0.;
        }

        static bool enableDissolvedGas()
        {
            return false;
        }

        static bool enableDissolvedGasInWater()
        {
            return false;
        }

        static bool enableVaporizedWater()
        {
            return false;
        }

        static bool enableVaporizedOil()
        {
            return false;
        }

    private:
        static bool isConsistent() {
            return eos_regions_.front().components.size() == NumComp;
        }

        // Every reservoir EOS region; a fluid system set up without a deck has one.
        static std::vector<EosRegion> eos_regions_;
        static bool eos_type_from_deck_;
        static std::array<Scalar, 5> lbc_coefficients_;
        static std::shared_ptr<WaterPvt> waterPvt_;

    public:
        static std::string printComponentParams() {
            std::string result = "Components Information:\n";
            for (const auto& param : eos_regions_.front().components) {
                result += fmt::format("Name: {}\n", param.name);
                result += fmt::format("Molar Mass: {} kg/mol\n", param.molar_mass);
                result += fmt::format("Critical Temperature: {} K\n", param.critic_temp);
                result += fmt::format("Critical Pressure: {} Pascal\n", param.critic_pres);
                result += fmt::format("Critical Volume: {} m^3/kmol\n", param.critic_vol);
                result += fmt::format("Acentric Factor: {}\n", param.acentric_factor);
                result += "---------------------------------\n";
            }
            return result;
        }
    };

    template <class Scalar, int NumComp, bool enableWater>
    std::vector<typename GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::EosRegion>
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::eos_regions_(1);

    template <class Scalar, int NumComp, bool enableWater>
    bool
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::eos_type_from_deck_ = false;

    template <class Scalar, int NumComp, bool enableWater>
    std::array<Scalar, 5>
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::lbc_coefficients_ =
        ViscosityModel::defaultLBCCoefficients();

    template <class Scalar, int NumComp, bool enableWater>
    std::shared_ptr<WaterPvtMultiplexer<Scalar> >
    GenericOilGasWaterFluidSystem<Scalar, NumComp, enableWater>::waterPvt_;

}
#endif // OPM_GENERIC_OIL_GAS_WATER_FLUIDSYSTEM_HPP
