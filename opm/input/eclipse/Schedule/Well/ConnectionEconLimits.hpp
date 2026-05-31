/*
  Copyright 2026 Equinor ASA.

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

#ifndef OPM_CONNECTION_ECON_LIMITS_HPP
#define OPM_CONNECTION_ECON_LIMITS_HPP

#include <limits>
#include <string>

namespace Opm {

    class DeckRecord;

    /// Per-connection economic limits (CECON keyword).
    struct ConnectionEconLimits
    {
        /// Workover procedure for CECON limits (no NONE option unlike WECON).
        enum class EconWorkover {
            CON  = 1, // CON
            CONP = 2, // +CON
            WELL = 3,
            PLUG = 4,
        };

        static std::string EconWorkover2String(EconWorkover enumValue);
        static EconWorkover EconWorkoverFromString(const std::string& stringValue);

        static constexpr double no_lower_rate_limit = std::numeric_limits<double>::lowest();

        /// Maximum water cut (water/liquid ratio); 0.0 means off.
        double max_water_cut{0.0};

        /// Maximum gas/oil ratio; 0.0 means off.
        double max_gas_oil_ratio{0.0};

        /// Maximum water/gas ratio; 0.0 means off.
        double max_water_gas_ratio{0.0};

        /// Workover applied when a ratio limit is violated.
        EconWorkover workover{EconWorkover::CON};

        /// Whether to check stopped wells.
        bool check_stopped_wells{false};

        /// Minimum oil production rate (SI); defaults to no_lower_rate_limit.
        double min_oil_rate{no_lower_rate_limit};

        /// Minimum gas production rate (SI); defaults to no_lower_rate_limit.
        double min_gas_rate{no_lower_rate_limit};

        /// SI equivalent of -1e+20 in LiquidSurfaceVolume/Time (deck "no limit" sentinel).
        double min_oil_rate_sentinel{no_lower_rate_limit};

        /// SI equivalent of -1e+20 in GasSurfaceVolume/Time (deck "no limit" sentinel).
        double min_gas_rate_sentinel{no_lower_rate_limit};

        /// Optional follow-on well.
        std::string followon_well{};

        ConnectionEconLimits() = default;

        /// Construct from a CECON deck record.
        explicit ConnectionEconLimits(const DeckRecord& record);

        bool operator==(const ConnectionEconLimits& other) const;
        bool operator!=(const ConnectionEconLimits& other) const
        {
            return !(*this == other);
        }

        bool onAnyEffectiveLimit() const;
        bool onMaxWaterCut() const     { return this->max_water_cut       > 0.0; }
        bool onMaxGasOilRatio() const  { return this->max_gas_oil_ratio   > 0.0; }
        bool onMaxWaterGasRatio() const{ return this->max_water_gas_ratio > 0.0; }
        bool onMinOilRate() const      { return this->min_oil_rate        > this->min_oil_rate_sentinel; }
        bool onMinGasRate() const      { return this->min_gas_rate        > this->min_gas_rate_sentinel; }

        static ConnectionEconLimits serializationTestObject();

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->max_water_cut);
            serializer(this->max_gas_oil_ratio);
            serializer(this->max_water_gas_ratio);
            serializer(this->workover);
            serializer(this->check_stopped_wells);
            serializer(this->min_oil_rate);
            serializer(this->min_gas_rate);
            serializer(this->min_oil_rate_sentinel);
            serializer(this->min_gas_rate_sentinel);
            serializer(this->followon_well);
        }
    };

} // namespace Opm

#endif // OPM_CONNECTION_ECON_LIMITS_HPP
