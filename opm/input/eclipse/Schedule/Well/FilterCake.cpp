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
            throw OpmInputError(fmt::format("Unknown geometry type {} is specified in WINJDAM keyword", str), location);
    }

    std::string FilterCake::filterCakeGeometryToString(const Opm::FilterCake::FilterCakeGeometry& geometry) {
        switch (geometry) {
            case FilterCakeGeometry::LINEAR:
                return "LINEAR";
            case FilterCakeGeometry::RADIAL:
                return "RADIAL";
            case FilterCakeGeometry::NONE:
                return "NONE";
            default:
                return "unknown FileterCakeGeometry type";
        }
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

    void FilterCake::applyCleanMultiplier(const double factor) {
        this->sf_multiplier *= factor;
    }

    FilterCake FilterCake::serializationTestObject() {
        FilterCake filter_cake;
        filter_cake.geometry = FilterCakeGeometry::LINEAR;
        filter_cake.perm = 1.e-8;
        filter_cake.poro = 0.2;
        filter_cake.radius = 0.1;
        filter_cake.flow_area = 20.;
        filter_cake.sf_multiplier = 0.2;

        return filter_cake;
    }

    std::string FilterCake::filterCakeToString(const FilterCake& fc) {
        std::string str = fmt::format("geometry type {}, perm {}, poro {}",
                                      filterCakeGeometryToString(fc.geometry), fc.perm, fc.poro);
        if (fc.radius.has_value()) {
            fmt::format_to(std::back_inserter(str), ", radius {}", fc.radius.value());
        } else {
            fmt::format_to(std::back_inserter(str), ", radius DEFAULT");
        }

        if (fc.flow_area.has_value()) {
            fmt::format_to(std::back_inserter(str), ", flow_area {}", fc.flow_area.value());
        } else {
            fmt::format_to(std::back_inserter(str), ", flow_area DEFAULT");
        }

        fmt::format_to(std::back_inserter(str), ", sf_multiplier {}.", fc.sf_multiplier);
        return str;
    }
} // end of Opm namespace

