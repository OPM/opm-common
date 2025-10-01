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
 * \copydoc Opm::EclEpsTwoPhaseLawParams
 */
#ifndef OPM_ECL_EPS_TWO_PHASE_LAW_PARAMS_HPP
#define OPM_ECL_EPS_TWO_PHASE_LAW_PARAMS_HPP

#include "EclEpsConfig.hpp"
#include "EclEpsScalingPoints.hpp"

#include <memory>
#include <cassert>

#include <opm/material/common/EnsureFinalized.hpp>

namespace Opm {
/*!
 * \ingroup FluidMatrixInteractions
 *
 * \brief A default implementation of the parameters for the material law adapter class
 *        which implements ECL endpoint scaling.
 */
template <class EffLawT>
class EclEpsTwoPhaseLawParams : public EnsureFinalized
{
    using EffLawParams = typename EffLawT::Params;
    using Scalar = typename EffLawParams::Traits::Scalar;

public:
    using Traits = typename EffLawParams::Traits;
    using ScalingPoints = EclEpsScalingPoints<Scalar>;

    EclEpsTwoPhaseLawParams()
    {
    }

    /*!
     * \brief Calculate all dependent quantities once the independent
     *        quantities of the parameter object have been set.
     */
    void finalize()
    {
#ifndef NDEBUG
        if (config_.enableSatScaling()) {
            assert(unscaledPoints_);
        }
        assert(effectiveLawParams_);
#endif
        EnsureFinalized :: finalize();
    }

    /*!
     * \brief Set the endpoint scaling configuration object.
     */
    void setConfig(const EclEpsConfig& value)
    { config_ = value; }

    /*!
     * \brief Returns the endpoint scaling configuration object.
     */
    const EclEpsConfig& config() const
    { return config_; }

    /*!
     * \brief Set the scaling points which are seen by the nested material law
     */
    void setUnscaledPoints(std::shared_ptr<ScalingPoints> value)
    { unscaledPoints_ = value.get(); }

    /*!
     * \brief Returns the scaling points which are seen by the nested material law
     */
    const ScalingPoints& unscaledPoints() const
    { return *unscaledPoints_; }

    /*!
     * \brief Set the scaling points which are seen by the physical model
     */
    void setScaledPoints(const ScalingPoints& value)
    { scaledPoints_ = value; }

    /*!
     * \brief Returns the scaling points which are seen by the physical model
     */
    const ScalingPoints& scaledPoints() const
    { return scaledPoints_; }

    /*!
     * \brief Returns the scaling points which are seen by the physical model
     */
    ScalingPoints& scaledPoints()
    { return scaledPoints_; }

    Scalar SnTrapped([[maybe_unused]] bool maximumTrapping) const
    {
        return 0.0;
    }

    Scalar SnStranded([[maybe_unused]] Scalar sg, [[maybe_unused]] Scalar krg) const
    {
        return 0.0;
    }

    Scalar SwTrapped() const
    {
        return 0.0;
    }

    /*!
     * \brief Sets the parameter object for the effective/nested material law.
     */
    void setEffectiveLawParams(std::shared_ptr<EffLawParams> value)
    { effectiveLawParams_ = value.get(); }

    /*!
     * \brief Returns the parameter object for the effective/nested material law.
     */
    const EffLawParams& effectiveLawParams() const
    { return *effectiveLawParams_; }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        // Do nothing, this class has no dynamic state!
        // It is still somewhat useful to have this function,
        // as then we do not need to embed that knowledge in
        // other classes that may contain this one.
    }

private:
    EffLawParams* effectiveLawParams_{};
    EclEpsConfig config_;
    ScalingPoints* unscaledPoints_{};
    ScalingPoints scaledPoints_;
};

} // namespace Opm

#endif
