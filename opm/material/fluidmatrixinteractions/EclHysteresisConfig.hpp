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
 * \copydoc Opm::EclHysteresisConfig
 */
#ifndef OPM_ECL_HYSTERESIS_CONFIG_HPP
#define OPM_ECL_HYSTERESIS_CONFIG_HPP

namespace Opm {

#if HAVE_ECL_INPUT
class Runspec;
#endif

/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Specifies the configuration used by the ECL kr/pC hysteresis code
 */
class EclHysteresisConfig
{
public:
    /*!
     * \brief Specify whether hysteresis is enabled or not.
     */
    void setEnableHysteresis(bool yesno)
    { enableHysteresis_ = yesno; }

    /*!
     * \brief Returns whether hysteresis is enabled.
     */
    bool enableHysteresis() const
    { return enableHysteresis_; }

    /*!
     * \brief Set the type of the hysteresis model which is used for capillary pressure.
     *
     * -1: capillary pressure hysteresis is disabled
     * 0: use the Killough model for capillary pressure hysteresis
     */
    void setPcHysteresisModel(int value)
    { pcHysteresisModel_ = value; }

    /*!
     * \brief Return the type of the hysteresis model which is used for capillary pressure.
     *
     * -1: capillary pressure hysteresis is disabled
     * 0: use the Killough model for capillary pressure hysteresis
     */
    int pcHysteresisModel() const
    { return pcHysteresisModel_; }

    bool enablePCHysteresis() const
    { return enableHysteresis() && pcHysteresisModel_ >= 0; }

    /*!
     * \brief Set the type of the hysteresis model which is used for relative permeability.
     *
     * -1: relperm hysteresis is disabled
     * 0: use the Carlson model for relative permeability hysteresis of the non-wetting
     *    phase and the drainage curve for the relperm of the wetting phase
     * 1: use the Carlson model for relative permeability hysteresis of the non-wetting
     *    phase and the imbibition curve for the relperm of the wetting phase
     * 2: use the Killough model for relative permeability hysteresis of the non-wetting
     *    phase and the drainage curve for the relperm of the wetting phase
     * 3: use the Killough model for relative permeability hysteresis of the non-wetting
     *    phase and the imbibition curve for the relperm of the wetting phase
     */
    void setKrHysteresisModel(int value)
    { krHysteresisModel_ = value; }

    bool enableWettingHysteresis() const
    { return enableHysteresis() && krHysteresisModel_ >= 4; }

    bool enableNonWettingHysteresis() const
    { return enableHysteresis() && krHysteresisModel_ >= 0; }

    /*!
     * \brief Return the type of the hysteresis model which is used for relative permeability.
     *
     * -1: relperm hysteresis is disabled
     * 0/1: use the Carlson model for relative permeability hysteresis
     * 2/3: use the Killough model for relative permeability hysteresis
     */
    int krHysteresisModel() const
    { return krHysteresisModel_; }

    /*!
     * \brief Regularisation parameter used for Killough model.
     *
     * default: 0.1
     */
    double modParamTrapped() const
    { return modParamTrapped_; }

    /*!
     * \brief Curvature parameter used for capillary pressure hysteresis.
     *
     * default: 0.1
     */
    double curvatureCapPrs() const
    { return curvatureCapPrs_; }

    /*!
     * \brief Returns whether hysteresis is enabled.
     */
    bool enableWagHysteresis() const
    { return enableWagHyst_; }

#if HAVE_ECL_INPUT
    /*!
     * \brief Reads all relevant material parameters form a cell of a parsed ECL deck.
     *
     * This requires that the opm-parser module is available.
     */
    void initFromState(const Runspec& runspec);
#endif

private:
    // enable hysteresis at all
    bool enableHysteresis_{false};

    // the capillary pressure and the relperm hysteresis models to be used
    int pcHysteresisModel_{-1};
    int krHysteresisModel_{-1};
    double modParamTrapped_{};
    double curvatureCapPrs_{};

    // WAG hysteresis
    bool enableWagHyst_{false};
};

} // namespace Opm

#endif
