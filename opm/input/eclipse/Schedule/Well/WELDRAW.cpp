/*
  Copyright (C) 2026 Equinor ASA

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

#include <opm/input/eclipse/Schedule/Well/WELDRAW.hpp>

#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/input/eclipse/Schedule/eval_uda.hpp>

#include <stdexcept>

#include <fmt/format.h>

namespace Opm {

    WELDRAW::WELDRAW(const Dimension& pressure_dim)
        : m_max_drawdown { 0.0, pressure_dim }
    {}

    WELDRAW::TargetPhase WELDRAW::TargetPhaseFromString(const std::string& stringValue)
    {
        if (stringValue == "LIQ") {
            return TargetPhase::LIQ;
        }

        if (stringValue == "GAS") {
            return TargetPhase::GAS;
        }

        throw std::invalid_argument {
            fmt::format("Invalid phase '{}' for WELDRAW. "
                        "Must be LIQ or GAS.", stringValue)
        };
    }

    WELDRAW::Mode WELDRAW::ModeFromString(const std::string& stringValue)
    {
        if (stringValue == "AVG") {
            return Mode::AVG;
        }

        if (stringValue == "MAX") {
            return Mode::MAX;
        }

        throw std::invalid_argument {
            fmt::format("Invalid drawdown calculation mode '{}' for "
                        "WELDRAW. Must be AVG or MAX.", stringValue)
        };
    }

    WELDRAW WELDRAW::serializationTestObject()
    {
        WELDRAW result;
        result.m_max_drawdown = UDAValue{1.0e5};
        result.m_phase = TargetPhase::GAS;
        result.m_mode = Mode::MAX;
        result.m_use_in_potentials = true;
        result.m_active = true;

        return result;
    }

    void WELDRAW::update(const DeckRecord& record, const Phase preferred_phase)
    {
        using Kw = ParserKeywords::WELDRAW;

        const auto& draw_item = record.getItem<Kw::MAX_DRAW>();
        if (!draw_item.hasValue(0) || draw_item.defaultApplied(0)) {
            // A defaulted maximum drawdown removes the limit.
            this->m_active = false;
        }
        else {
            this->m_max_drawdown = draw_item.get<UDAValue>(0);
            // A non-positive drawdown limit is treated as no limit.
            this->m_active = !(this->m_max_drawdown.is<double>() &&
                               (this->m_max_drawdown.get<double>() <= 0.0));
        }

        const auto& phase_item = record.getItem<Kw::PHASE>();
        if (phase_item.hasValue(0) && !phase_item.defaultApplied(0)) {
            this->m_phase = TargetPhaseFromString(phase_item.getTrimmedString(0));
        }
        else {
            this->m_phase = (preferred_phase == Phase::GAS)
                ? TargetPhase::GAS : TargetPhase::LIQ;
        }

        this->m_use_in_potentials =
            DeckItem::to_bool(record.getItem<Kw::USE_LIMIT>().getTrimmedString(0));

        this->m_mode =
            ModeFromString(record.getItem<Kw::GRID_BLOCKS>().getTrimmedString(0));
    }

    double WELDRAW::maxDrawdown(const std::string& well_name,
                                const SummaryState& st,
                                const double udq_undefined) const
    {
        return UDA::eval_well_uda(this->m_max_drawdown, well_name, st, udq_undefined);
    }

    bool WELDRAW::operator==(const WELDRAW& other) const
    {
        return (this->m_max_drawdown == other.m_max_drawdown)
            && (this->m_phase == other.m_phase)
            && (this->m_mode == other.m_mode)
            && (this->m_use_in_potentials == other.m_use_in_potentials)
            && (this->m_active == other.m_active);
    }

} // namespace Opm
