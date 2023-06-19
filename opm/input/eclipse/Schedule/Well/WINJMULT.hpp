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

#ifndef OPM_WINJMULT_HPP
#define OPM_WINJMULT_HPP

#include <limits>


namespace Opm {

class KeywordLocation;

struct InjMult {

    enum class InjMultMode {
        WREV,
        CREV,
        CIRR,
        NONE,
    };

    bool is_active {false};
    double fracture_pressure {std::numeric_limits<double>::max()};
    double multiplier_gradient {0.};

    static InjMultMode injMultModeFromString(const std::string& str, const KeywordLocation& location);

    bool active() const;

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(is_active);
        serializer(fracture_pressure);
        serializer(multiplier_gradient);
    }

    bool operator==(const InjMult& rhs) const;

    static InjMult serializationTestObject();
    static std::string InjMultToString(const InjMult&);
};

}

#endif // OPM_WINJMULT_HPP
