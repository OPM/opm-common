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

#ifndef OPM_FILTERCAKE_HPP
#define OPM_FILTERCAKE_HPP

#include <string>
#include <optional>

namespace Opm {

    class DeckRecord;
    class KeywordLocation;

    struct FilterCake {

        enum class FilterCakeGeometry {
            LINEAR,
            RADIAL,
            LINRAD,
            NONE,
        };

        static FilterCakeGeometry filterCakeGeometryFromString(const std::string& str, const KeywordLocation& location);
        static std::string filterCakeGeometryToString(const FilterCakeGeometry& geometry);

        FilterCakeGeometry geometry{FilterCakeGeometry::NONE};
        double perm{0.};
        double poro{0.};
        std::optional<double> radius;
        std::optional<double> flow_area;
        // skin factor multiplier
        // it is controlled by keyword WINJCLN
        mutable double sf_multiplier{1.};

        FilterCake() = default;

        explicit FilterCake(const DeckRecord& record, const KeywordLocation& location);

        template<class Serializer>
        void serializeOp(Serializer& serializer) {
            serializer(geometry);
            serializer(perm);
            serializer(poro);
            serializer(radius);
            serializer(flow_area);
            serializer(sf_multiplier);
        }

        static FilterCake serializationTestObject();

        bool operator==(const FilterCake& other) const;

        void applyCleanMultiplier(const double factor);

        static std::string filterCakeToString(const FilterCake& fc);
    };

}

#endif //OPM_FILTERCAKE_HPP
