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
 * \copydoc Opm::EclEpsTwoPhaseLawPoints
 */
#ifndef OPM_ECL_EPS_SCALING_POINTS_HPP
#define OPM_ECL_EPS_SCALING_POINTS_HPP

#include <array>
#include <vector>

namespace Opm {

class EclEpsConfig;
enum class EclTwoPhaseSystemType;

#if HAVE_ECL_INPUT
class EclipseState;
class EclEpsGridProperties;

namespace satfunc {
struct RawTableEndPoints;
struct RawFunctionValues;
}
#endif

/*!
 * \brief This structure represents all values which can be possibly used as scaling
 *        points in the endpoint scaling code.
 *
 * Depending on the exact configuration, some of these quantities are not used as actual
 * scaling points. It is easier to extract all of them at once, though.
 */
template <class Scalar>
struct EclEpsScalingPointsInfo
{
    // connate saturations
    Scalar Swl; // water
    Scalar Sgl; // gas

    // critical saturations
    Scalar Swcr; // water
    Scalar Sgcr; // gas
    Scalar Sowcr; // oil for the oil-water system
    Scalar Sogcr; // oil for the gas-oil system

    // maximum saturations
    Scalar Swu; // water
    Scalar Sgu; // gas

    // maximum capillary pressures
    Scalar maxPcow; // maximum capillary pressure of the oil-water system
    Scalar maxPcgo; // maximum capillary pressure of the gas-oil system

    // the Leverett capillary pressure scaling factors. (those only make sense for the
    // scaled points, for the unscaled ones they are 1.0.)
    Scalar pcowLeverettFactor;
    Scalar pcgoLeverettFactor;

    // Scaled relative permeabilities at residual displacing saturation
    Scalar Krwr;  // water
    Scalar Krgr;  // gas
    Scalar Krorw; // oil in water-oil system
    Scalar Krorg; // oil in gas-oil system

    // maximum relative permabilities
    Scalar maxKrw; // maximum relative permability of water
    Scalar maxKrow; // maximum relative permability of oil in the oil-water system
    Scalar maxKrog; // maximum relative permability of oil in the gas-oil system
    Scalar maxKrg; // maximum relative permability of gas

    bool operator==(const EclEpsScalingPointsInfo<Scalar>& data) const
    {
        return Swl == data.Swl &&
               Sgl == data.Sgl &&
               Swcr == data.Swcr &&
               Sgcr == data.Sgcr &&
               Sowcr == data.Sowcr &&
               Sogcr == data.Sogcr &&
               Swu == data.Swu &&
               Sgu == data.Sgu &&
               maxPcow == data.maxPcow &&
               maxPcgo == data.maxPcgo &&
               pcowLeverettFactor == data.pcowLeverettFactor &&
               pcgoLeverettFactor == data.pcgoLeverettFactor &&
               Krwr == data.Krwr &&
               Krgr == data.Krgr &&
               Krorw == data.Krorw &&
               Krorg == data.Krorg &&
               maxKrw == data.maxKrw &&
               maxKrow == data.maxKrow &&
               maxKrog == data.maxKrog &&
               maxKrg == data.maxKrg;
    }

    void print() const;

#if HAVE_ECL_INPUT
    /*!
     * \brief Extract the values of the unscaled scaling parameters.
     *
     * I.e., the values which are used for the nested Fluid-Matrix interactions and which
     * are produced by them.
     */
    void extractUnscaled(const satfunc::RawTableEndPoints& rtep,
                         const satfunc::RawFunctionValues& rfunc,
                         const std::vector<double>::size_type satRegionIdx);

    void update(Scalar& targetValue, const double * value_ptr)
    {
        if (value_ptr)
            targetValue = *value_ptr;
    }

    /*!
     * \brief Extract the values of the scaled scaling parameters.
     *
     * I.e., the values which are "seen" by the physical model.
     */
    void extractScaled(const EclipseState& eclState,
                       const EclEpsGridProperties& epsProperties,
                       unsigned activeIndex);
#endif

private:
    void extractGridPropertyValue_(Scalar& targetValue,
                                   const std::vector<double>* propData,
                                   unsigned cartesianCellIdx)
    {
        if (!propData)
            return;

        targetValue = (*propData)[cartesianCellIdx];
    }
};

/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief Represents the points on the X and Y axis to be scaled if endpoint scaling is
 *        used.
 */
template <class Scalar>
class EclEpsScalingPoints
{
public:
    /*!
     * \brief Assigns the scaling points which actually ought to be used.
     */
    void init(const EclEpsScalingPointsInfo<Scalar>& epsInfo,
              const EclEpsConfig& config,
              EclTwoPhaseSystemType epsSystemType);

    /*!
     * \brief Sets an saturation value for capillary pressure saturation scaling
     */
    void setSaturationPcPoint(unsigned pointIdx, Scalar value)
    { saturationPcPoints_[pointIdx] = value; }

    /*!
     * \brief Returns the points used for capillary pressure saturation scaling
     */
    const std::array<Scalar, 3>& saturationPcPoints() const
    { return saturationPcPoints_; }

    /*!
     * \brief Sets an saturation value for wetting-phase relperm saturation scaling
     */
    void setSaturationKrwPoint(unsigned pointIdx, Scalar value)
    { saturationKrwPoints_[pointIdx] = value; }

    /*!
     * \brief Returns the points used for wetting phase relperm saturation scaling
     */
    const std::array<Scalar, 3>& saturationKrwPoints() const
    { return saturationKrwPoints_; }

    /*!
     * \brief Sets an saturation value for non-wetting phase relperm saturation scaling
     */
    void setSaturationKrnPoint(unsigned pointIdx, Scalar value)
    { saturationKrnPoints_[pointIdx] = value; }

    /*!
     * \brief Returns the points used for non-wetting phase relperm saturation scaling
     */
    const std::array<Scalar, 3>& saturationKrnPoints() const
    { return saturationKrnPoints_; }

    /*!
     * \brief Sets the maximum capillary pressure
     */
    void setMaxPcnw(Scalar value)
    { maxPcnwOrLeverettFactor_ = value; }

    /*!
     * \brief Returns the maximum capillary pressure
     */
    Scalar maxPcnw() const
    { return maxPcnwOrLeverettFactor_; }

    /*!
     * \brief Sets the Leverett scaling factor for capillary pressure
     */
    void setLeverettFactor(Scalar value)
    { maxPcnwOrLeverettFactor_ = value; }

    /*!
     * \brief Returns the Leverett scaling factor for capillary pressure
     */
    Scalar leverettFactor() const
    { return maxPcnwOrLeverettFactor_; }

    /*!
     * \brief Set wetting-phase relative permeability at residual saturation
     * of non-wetting phase.
     */
    void setKrwr(Scalar value)
    { this->Krwr_ = value; }

    /*!
     * \brief Returns wetting-phase relative permeability at residual
     * saturation of non-wetting phase.
     */
    Scalar krwr() const
    { return this->Krwr_; }

    /*!
     * \brief Sets the maximum wetting phase relative permeability
     */
    void setMaxKrw(Scalar value)
    { maxKrw_ = value; }

    /*!
     * \brief Returns the maximum wetting phase relative permeability
     */
    Scalar maxKrw() const
    { return maxKrw_; }

    /*!
     * \brief Set non-wetting phase relative permeability at residual
     * saturation of wetting phase.
     */
    void setKrnr(Scalar value)
    { this->Krnr_ = value; }

    /*!
     * \brief Returns non-wetting phase relative permeability at residual
     * saturation of wetting phase.
     */
    Scalar krnr() const
    { return this->Krnr_; }

    /*!
     * \brief Sets the maximum wetting phase relative permeability
     */
    void setMaxKrn(Scalar value)
    { maxKrn_ = value; }

    /*!
     * \brief Returns the maximum wetting phase relative permeability
     */
    Scalar maxKrn() const
    { return maxKrn_; }

    void print() const;

private:
    // Points used for vertical scaling of capillary pressure
    Scalar maxPcnwOrLeverettFactor_;

    // Maximum wetting phase relative permability value.
    Scalar maxKrw_;

    // Scaled wetting phase relative permeability value at residual
    // saturation of non-wetting phase.
    Scalar Krwr_;

    // Maximum non-wetting phase relative permability value
    Scalar maxKrn_;

    // Scaled non-wetting phase relative permeability value at residual
    // saturation of wetting phase.
    Scalar Krnr_;

    // The the points used for saturation ("x-axis") scaling of capillary pressure
    std::array<Scalar, 3> saturationPcPoints_;

    // The the points used for saturation ("x-axis") scaling of wetting phase relative permeability
    std::array<Scalar, 3> saturationKrwPoints_;

    // The the points used for saturation ("x-axis") scaling of non-wetting phase relative permeability
    std::array<Scalar, 3> saturationKrnPoints_;
};

} // namespace Opm

#endif
