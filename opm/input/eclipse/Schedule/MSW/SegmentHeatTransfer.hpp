/*
  Copyright 2026 Equinor.

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

#ifndef SEGMENT_HEAT_TRANSFER_HPP_HEADER_INCLUDED
#define SEGMENT_HEAT_TRANSFER_HPP_HEADER_INCLUDED

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

    class DeckKeyword;
    class DeckRecord;

} // namespace Opm

namespace Opm {

    // A single heat transfer coefficient associated with a segment of a
    // multisegment well, as specified by the WSEGHEAT keyword.
    class SegmentHeatTransferCoeff
    {
    public:
        // The destination of the heat transfer.  NONE is not a coefficient in
        // its own right; it is only used to clear all coefficients.
        enum class Type {
            COMP,   // heat transfer to a previously defined completion
            SEG,    // heat transfer to another segment
            TEMP,   // heat transfer to a fixed external temperature
            NONE,   // remove the segment from the list of source/sinks
        };

        // How a WSEGHEAT record modifies the set of coefficients already
        // associated with a segment.
        enum class Operation {
            REPLACE_ALL, // replace all existing coefficients with this one
            ADD,         // add to (or update within) the existing set ('+')
            REMOVE,      // remove the matching coefficient ('-')
            CLEAR,       // remove all coefficients (type NONE)
        };

        SegmentHeatTransferCoeff() = default;
        SegmentHeatTransferCoeff(Type type, const DeckRecord& record);

        static SegmentHeatTransferCoeff serializationTestObject();

        // Decode the WSEGHEAT type string (item 4), e.g. "SEG", "COMP+" or
        // "TEMP-", into a base type and the operation it implies.
        static std::pair<Type, Operation> typeFromString(const std::string& s);

        static std::string typeToString(Type type);

        Type type() const { return m_type; }
        double thermalResistance() const { return m_thermal_resistance; }
        int targetSegment() const { return m_target_segment; }
        double interpolationConstant() const { return m_interpolation_constant; }

        // External fixed temperature (item 7).  Only set for Type::TEMP;
        // nullopt otherwise.
        const std::optional<double>& temperature() const { return m_temperature; }

        // Contact length (item 9).  Nullopt when the item was defaulted, in
        // which case the value should be taken as the mean length of the
        // adjacent segments.
        const std::optional<double>& contactLength() const { return m_contact_length; }

        bool operator==(const SegmentHeatTransferCoeff&) const = default;

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_type);
            serializer(m_thermal_resistance);
            serializer(m_target_segment);
            serializer(m_temperature);
            serializer(m_interpolation_constant);
            serializer(m_contact_length);
        }

    private:
        Type m_type{Type::COMP};

        // Specific thermal resistance (item 5).
        double m_thermal_resistance{0.0};

        // Target segment number (item 6), only meaningful for Type::SEG.
        int m_target_segment{0};

        // External fixed temperature (item 7); see temperature().
        std::optional<double> m_temperature{};

        // Interpolation constant (item 8), only meaningful for Type::SEG.
        double m_interpolation_constant{0.5};

        // Contact length (item 9); see contactLength().
        std::optional<double> m_contact_length{};
    };

    // One parsed WSEGHEAT record: the segment range it applies to, the
    // operation it performs on those segments' coefficient sets, and the
    // coefficient itself (unused when the operation is CLEAR).
    struct SegmentHeatTransferRecord
    {
        int segment1{};
        int segment2{};
        SegmentHeatTransferCoeff::Operation operation{SegmentHeatTransferCoeff::Operation::CLEAR};
        SegmentHeatTransferCoeff coefficient{};

        bool operator==(const SegmentHeatTransferRecord&) const = default;

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(segment1);
            serializer(segment2);
            serializer(operation);
            serializer(coefficient);
        }
    };

    // Parse a WSEGHEAT keyword into a deck-ordered list of (well-name pattern,
    // record) pairs.  Deck order is preserved deliberately: WSEGHEAT records are
    // incremental (replace, '+' add, '-' remove, NONE clear) and must be applied
    // in deck order per well.  Grouping by well name here would lose that order
    // when patterns overlap, so it is left to the consumer, which can expand the
    // patterns to concrete well names first.
    std::vector<std::pair<std::string, SegmentHeatTransferRecord>>
    segmentHeatTransferFromWSEGHEAT(const DeckKeyword& keyword);

} // namespace Opm

#endif // SEGMENT_HEAT_TRANSFER_HPP_HEADER_INCLUDED
