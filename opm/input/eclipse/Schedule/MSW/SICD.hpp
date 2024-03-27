/*
  Copyright 2017 SINTEF Digital, Mathematics and Cybernetics.
  Copyright 2019 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SPIRALICD_HPP_HEADER_INCLUDED
#define SPIRALICD_HPP_HEADER_INCLUDED

#include <opm/input/eclipse/Schedule/MSW/icd.hpp>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

    class DeckRecord;
    class DeckKeyword;

} // namespace Opm

namespace Opm { namespace RestartIO {
    struct RstSegment;
}} // namespace Opm::RestartIO

namespace Opm {

    class SICD
    {
    public:
        SICD() = default;
        explicit SICD(const DeckRecord& record);
        explicit SICD(const RestartIO::RstSegment& rstSegment);

        SICD(double strength,
             double length,
             double densityCalibration,
             double viscosityCalibration,
             double criticalValue,
             double widthTransitionRegion,
             double maxViscosityRatio,
             int methodFlowScaling,
             const std::optional<double>& maxAbsoluteRate,
             ICDStatus status,
             double scalingFactor);

        virtual ~SICD() = default;

        static SICD serializationTestObject();

        // the function will return a map
        // [
        //     "WELL1" : [<seg1, sicd1>, <seg2, sicd2> ...]
        //     ....
        static std::map<std::string, std::vector<std::pair<int, SICD>>>
        fromWSEGSICD(const DeckKeyword& wsegsicd);

        const std::optional<double>& maxAbsoluteRate() const;
        ICDStatus status() const;
        double strength() const;
        double length() const;
        double densityCalibration() const;
        double viscosityCalibration() const;
        double criticalValue() const;
        double widthTransitionRegion() const;
        double maxViscosityRatio() const;
        int methodFlowScaling() const;

        void updateScalingFactor(const double segment_length, const double completion_length);
        double scalingFactor() const;
        int ecl_status() const;
        bool operator==(const SICD& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_strength);
            serializer(m_length);
            serializer(m_density_calibration);
            serializer(m_viscosity_calibration);
            serializer(m_critical_value);
            serializer(m_width_transition_region);
            serializer(m_max_viscosity_ratio);
            serializer(m_method_flow_scaling);
            serializer(m_max_absolute_rate);
            serializer(m_status);
            serializer(m_scaling_factor);
        }

    private:
        double m_strength { 0.0 };
        double m_length { 0.0 };
        double m_density_calibration { 0.0 };
        double m_viscosity_calibration { 0.0 };
        double m_critical_value { 0.0 };
        double m_width_transition_region { 0.0 };
        double m_max_viscosity_ratio { 0.0 };
        int m_method_flow_scaling { 0 };
        std::optional<double> m_max_absolute_rate {};
        ICDStatus m_status { ICDStatus::SHUT };

        // scaling factor is the only one can not be gotten from deck
        // directly, needs to be updated afterwards
        std::optional<double> m_scaling_factor { 1.0 };
    };

} // namespace Opm

#endif // SPIRALICD_HPP_HEADER_INCLUDED
