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
 * \copydoc Opm::EclThermalLawManager
 */
#ifndef OPM_ECL_THERMAL_LAW_MANAGER_HPP
#define OPM_ECL_THERMAL_LAW_MANAGER_HPP

#if ! HAVE_ECL_INPUT
#error "Eclipse input support in opm-common is required to use the ECL thermal law manager!"
#endif

#include "EclSolidEnergyLawMultiplexer.hpp"
#include "EclSolidEnergyLawMultiplexerParams.hpp"

#include "EclThermalConductionLawMultiplexer.hpp"
#include "EclThermalConductionLawMultiplexerParams.hpp"

#include <vector>

namespace Opm {

class EclipseState;

/*!
 * \ingroup fluidmatrixinteractions
 *
 * \brief Provides an simple way to create and manage the thermal law objects
 *        for a complete ECL deck.
 */
template <class Scalar, class FluidSystem>
class EclThermalLawManager
{
public:
    using SolidEnergyLaw = EclSolidEnergyLawMultiplexer<Scalar, FluidSystem>;
    using SolidEnergyLawParams = typename SolidEnergyLaw::Params;
    using HeatcrLawParams = typename SolidEnergyLawParams::HeatcrLawParams;
    using SpecrockLawParams = typename SolidEnergyLawParams::SpecrockLawParams;

    using ThermalConductionLaw = EclThermalConductionLawMultiplexer<Scalar, FluidSystem>;
    using ThermalConductionLawParams = typename ThermalConductionLaw::Params;

    void initParamsForElements(const EclipseState& eclState, size_t numElems);

    const SolidEnergyLawParams& solidEnergyLawParams(unsigned elemIdx) const;

    const ThermalConductionLawParams& thermalConductionLawParams(unsigned elemIdx) const;

private:
    /*!
     * \brief Initialize the parameters for the solid energy law using using HEATCR and friends.
     */
    void initHeatcr_(const EclipseState& eclState, size_t numElems);

    /*!
     * \brief Initialize the parameters for the solid energy law using using SPECROCK and friends.
     */
    void initSpecrock_(const EclipseState& eclState, size_t numElems);

    /*!
     * \brief Specify the solid energy law by setting heat capacity of rock to 0
     */
    void initNullRockEnergy_();

    /*!
     * \brief Initialize the parameters for the thermal conduction law using THCONR and friends.
     */
    void initThconr_(const EclipseState& eclState, size_t numElems);

    /*!
     * \brief Initialize the parameters for the thermal conduction law using THCROCK and friends.
     */
    void initThc_(const EclipseState& eclState, size_t numElems);

    /*!
     * \brief Disable thermal conductivity
     */
    void initNullCond_();

private:
    EclThermalConductionApproach thermalConductivityApproach_ = EclThermalConductionApproach::Undefined;
    EclSolidEnergyApproach solidEnergyApproach_ = EclSolidEnergyApproach::Undefined;

    std::vector<unsigned> elemToSatnumIdx_;

    std::vector<SolidEnergyLawParams> solidEnergyLawParams_;
    std::vector<ThermalConductionLawParams> thermalConductionLawParams_;
};
} // namespace Opm

#endif
