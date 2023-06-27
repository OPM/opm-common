/*
  Copyright 2023 Equinor.

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

#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>
#include <opm/input/eclipse/Schedule/Well/FilterCake.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <fmt/format.h>

namespace Opm {
    FilterCake::FilterCakeGeometry  FilterCake::filterCakeGeometryFromString(const std::string& str,
                                                                             const KeywordLocation& location) {
        if (str == "LINEAR")
            return FilterCakeGeometry::LINEAR;
        else if (str == "RADIAL")
            return FilterCakeGeometry::RADIAL;
        else
            throw OpmInputError(fmt::format("Unknown mode {} is specified in WINJDAM keyword", str), location);
    }


    FilterCake::FilterCake(const DeckRecord& record, const KeywordLocation& location) {
        this->geometry = filterCakeGeometryFromString(record.getItem<ParserKeywords::WINJDAM::GEOMETRY>().getTrimmedString(0),
                                                      location);
        this->perm = record.getItem<ParserKeywords::WINJDAM::FILTER_CAKE_PERM>().getSIDouble(0);
        this->poro = record.getItem<ParserKeywords::WINJDAM::FILTER_CAKE_PORO>().getSIDouble(0);

        const auto& item_radius = record.getItem<ParserKeywords::WINJDAM::FILTER_CAKE_RADIUS>();
        if (!item_radius.defaultApplied(0)) {
            this->radius = item_radius.getSIDouble(0);
        }

        const auto& item_area = record.getItem<ParserKeywords::WINJDAM::FILTER_CAKE_AREA>();
        if (!item_area.defaultApplied(0)) {
            this->flow_area = item_area.getSIDouble(0);
        }
    }

    bool FilterCake::operator==(const FilterCake& other) const {
        return geometry == other.geometry
               && perm == other.perm
               && poro == other.poro
               && radius == other.radius
               && flow_area == other.flow_area
               && sf_multiplier == other.sf_multiplier;
    }

    bool FilterCake::active() const {
        return this->geometry != FilterCakeGeometry::NONE;
    }

    void FilterCake::applyCleanMultiplier(const double factor) {
        this->sf_multiplier *= factor;
    }
} // end of Opm namespace

