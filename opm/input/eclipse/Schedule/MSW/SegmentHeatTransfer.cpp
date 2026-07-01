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

#include <opm/input/eclipse/Schedule/MSW/SegmentHeatTransfer.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <stdexcept>
#include <string>

#include <fmt/format.h>

namespace Opm {

    SegmentHeatTransfer::SegmentHeatTransfer(const Type type, const DeckRecord& record)
        : m_type(type)
    {
        using Kw = ParserKeywords::WSEGHEAT;

        // Items 5 and 6 have no default and are left at their member defaults
        // when not specified (e.g. for removal records).  Whether either item
        // is required is enforced in segmentHeatTransferFromWSEGHEAT().
        if (record.getItem<Kw::THERMAL_RESISTANCE>().hasValue(0)) {
            this->m_thermal_resistance =
                record.getItem<Kw::THERMAL_RESISTANCE>().getSIDouble(0);
        }

        if (record.getItem<Kw::TARGET_SEGMENT>().hasValue(0)) {
            this->m_target_segment =
                record.getItem<Kw::TARGET_SEGMENT>().get<int>(0);
        }

        if (record.getItem<Kw::TEMPERATURE>().hasValue(0)) {
            this->m_temperature = record.getItem<Kw::TEMPERATURE>().getSIDouble(0);
        }

        this->m_interpolation_constant =
            record.getItem<Kw::INTERPOLATION_CONSTANT>().get<double>(0);

        // Contact length (item 9) is left unset when defaulted, in which case
        // the mean length of the adjacent segments should be used instead.
        if (record.getItem<Kw::CONTACT_LENGTH>().hasValue(0)) {
            this->m_contact_length = record.getItem<Kw::CONTACT_LENGTH>().getSIDouble(0);
        }
    }

    SegmentHeatTransfer SegmentHeatTransfer::serializationTestObject()
    {
        SegmentHeatTransfer result;
        result.m_type = Type::SEG;
        result.m_thermal_resistance = 0.02;
        result.m_target_segment = 24;
        result.m_temperature = 350.0;
        result.m_interpolation_constant = 0.5;
        result.m_contact_length = 12.5;
        return result;
    }

    std::pair<SegmentHeatTransfer::Type, SegmentHeatTransfer::Operation>
    SegmentHeatTransfer::typeFromString(const std::string& s)
    {
        if (s == "NONE") {
            return { Type::NONE, Operation::CLEAR };
        }

        // The base type may be followed by '+' (add) or '-' (remove).
        auto operation = Operation::REPLACE_ALL;
        std::string base = s;
        if (!base.empty()) {
            if (base.back() == '+') {
                operation = Operation::ADD;
                base.pop_back();
            }
            else if (base.back() == '-') {
                operation = Operation::REMOVE;
                base.pop_back();
            }
        }

        if (base == "COMP") { return { Type::COMP, operation }; }
        if (base == "SEG")  { return { Type::SEG,  operation }; }
        if (base == "TEMP") { return { Type::TEMP, operation }; }

        throw std::invalid_argument {
            fmt::format("Unknown WSEGHEAT heat transfer coefficient type '{}'", s)
        };
    }

    std::string SegmentHeatTransfer::typeToString(const Type type)
    {
        switch (type) {
        case Type::COMP: return "COMP";
        case Type::SEG:  return "SEG";
        case Type::TEMP: return "TEMP";
        case Type::NONE: return "NONE";
        }

        throw std::invalid_argument("Unhandled SegmentHeatTransfer::Type value");
    }

    std::vector<std::pair<std::string, SegmentHeatTransferRecord>>
    segmentHeatTransferFromWSEGHEAT(const DeckKeyword& keyword)
    {
        using Kw = ParserKeywords::WSEGHEAT;

        auto res = std::vector<std::pair<std::string, SegmentHeatTransferRecord>>{};

        for (const auto& record : keyword) {
            const auto well_name = record.getItem<Kw::WELL>().getTrimmedString(0);

            const auto [type, operation] = SegmentHeatTransfer::typeFromString
                (record.getItem<Kw::TYPE>().getTrimmedString(0));

            auto rec = SegmentHeatTransferRecord{};
            rec.segment1 = record.getItem<Kw::SEGMENT1>().get<int>(0);
            rec.segment2 = record.getItem<Kw::SEGMENT2>().get<int>(0);
            rec.operation = operation;

            // The record applies to the inclusive segment range [segment1,
            // segment2].  A non-positive first segment or an inverted range is
            // self-inconsistent and can never match any well, so reject it here
            // rather than let it be silently ignored downstream.
            if ((rec.segment1 < 1) || (rec.segment2 < rec.segment1)) {
                throw OpmInputError {
                    fmt::format("An invalid WSEGHEAT segment range {} to {} was "
                                "specified for well {}; a valid range satisfies "
                                "1 <= item 2 (SEGMENT1) <= item 3 (SEGMENT2).",
                                rec.segment1, rec.segment2, well_name),
                    keyword.location()
                };
            }

            // For CLEAR (type NONE) only the operation matters; otherwise the
            // record carries a coefficient to set/add/remove.
            if (operation != SegmentHeatTransfer::Operation::CLEAR) {
                rec.coefficient = SegmentHeatTransfer{type, record};

                // Whether this record sets or adds a coefficient (a bare type
                // or one suffixed with '+').  A removal record ('-') only needs
                // to identify an existing coefficient and therefore need not
                // repeat the values that define it.
                const bool is_set =
                    (operation == SegmentHeatTransfer::Operation::REPLACE_ALL) ||
                    (operation == SegmentHeatTransfer::Operation::ADD);

                // The specific thermal resistance (item 5) is mandatory whenever
                // a coefficient is set or added.
                if (is_set && !record.getItem<Kw::THERMAL_RESISTANCE>().hasValue(0)) {
                    throw OpmInputError {
                        fmt::format("A {} heat transfer coefficient for well {} "
                                    "segments {} to {} requires a specific thermal "
                                    "resistance in item 5.",
                                    SegmentHeatTransfer::typeToString(type),
                                    well_name, rec.segment1, rec.segment2),
                        keyword.location()
                    };
                }

                // A fixed external temperature (item 7) is mandatory when a TEMP
                // coefficient is set or added.
                if (is_set && (type == SegmentHeatTransfer::Type::TEMP) &&
                    !rec.coefficient.temperature().has_value())
                {
                    throw OpmInputError {
                        fmt::format("A TEMP heat transfer coefficient for well {} "
                                    "segments {} to {} requires an external "
                                    "temperature in item 7.",
                                    well_name, rec.segment1, rec.segment2),
                        keyword.location()
                    };
                }

                // The target segment (item 6) identifies a SEG coefficient and
                // is therefore required for every SEG record, removal included.
                if ((type == SegmentHeatTransfer::Type::SEG) &&
                    (rec.coefficient.targetSegment() < 1))
                {
                    throw OpmInputError {
                        fmt::format("A SEG heat transfer coefficient for well {} "
                                    "segments {} to {} requires a target segment "
                                    "in item 6.",
                                    well_name, rec.segment1, rec.segment2),
                        keyword.location()
                    };
                }
            }

            res.emplace_back(well_name, rec);
        }

        return res;
    }

} // namespace Opm
