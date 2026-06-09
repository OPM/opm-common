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

#ifndef OPM_FIELDSEP_HPP
#define OPM_FIELDSEP_HPP

#include <cstddef>
#include <optional>
#include <vector>

namespace Opm {
    class DeckKeyword;
    class DeckRecord;
    class KeywordLocation;
} // namespace Opm

namespace Opm {

    /// One stage of a field separator (a single FIELDSEP record).
    class FieldSepRecord {
    public:
        FieldSepRecord() = default;
        FieldSepRecord(const DeckRecord& record, const KeywordLocation& location);

        static FieldSepRecord serializationTestObject();

        /// Stage index (FIELDSEP records are given in increasing order).
        int stage() const;
        /// Stage temperature (SI).
        double temperature() const;
        /// Stage pressure (SI).
        double pressure() const;
        /// Liquid destination: 0 => next stage (stock tank for the last
        /// stage); -1 => add to stock-tank oil.
        int liquidDestination() const;
        /// Vapour destination: 0 => stock tank / surface gas.
        int vaporDestination() const;
        /// K-value table number (0 => use the equation of state).
        int kvalueTableNumber() const;
        /// Gas-plant table number, references a GPTABLE (0 => use the EoS).
        int gasPlantTableNumber() const;
        /// Surface equation-of-state region number.  Nullopt if defaulted:
        /// 0 is a valid (0-based) EoS number, so absence is expressed directly.
        std::optional<int> surfaceEosNumber() const;
        /// NGL-density-evaluation temperature (SI).  Nullopt if defaulted.
        std::optional<double> nglDensityTemperature() const;
        /// NGL-density-evaluation pressure (SI).  Nullopt if defaulted.
        std::optional<double> nglDensityPressure() const;

        bool operator==(const FieldSepRecord& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_stage);
            serializer(m_temperature);
            serializer(m_pressure);
            serializer(m_liquid_destination);
            serializer(m_vapor_destination);
            serializer(m_kvalue_table_number);
            serializer(m_gas_plant_table_number);
            serializer(m_surface_eos_number);
            serializer(m_ngl_density_temperature);
            serializer(m_ngl_density_pressure);
        }

    private:
        int m_stage = 0;
        double m_temperature = 0.0;
        double m_pressure = 0.0;
        int m_liquid_destination = 0;
        int m_vapor_destination = 0;
        int m_kvalue_table_number = 0;
        int m_gas_plant_table_number = 0;
        std::optional<int> m_surface_eos_number{};
        std::optional<double> m_ngl_density_temperature{};
        std::optional<double> m_ngl_density_pressure{};
    };

    /// A multi-stage field separator (the FIELDSEP keyword).
    ///
    /// Mirrors the EquilContainer interface, but is a dedicated container: a
    /// field separator needs no Phases / compositional / region arguments.
    class FieldSep {
    public:
        FieldSep() = default;
        explicit FieldSep(const DeckKeyword& keyword);

        static FieldSep serializationTestObject();

        const FieldSepRecord& getRecord(std::size_t id) const;

        std::size_t size()  const { return this->m_records.size(); }
        bool        empty() const { return this->m_records.empty(); }

        auto begin() const { return this->m_records.begin(); }
        auto end()   const { return this->m_records.end(); }

        bool operator==(const FieldSep& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_records);
        }

    private:
        std::vector<FieldSepRecord> m_records;
    };

} // namespace Opm

#endif // OPM_FIELDSEP_HPP
