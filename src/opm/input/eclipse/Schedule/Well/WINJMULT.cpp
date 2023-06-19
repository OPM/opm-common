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


#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Schedule/Well/WINJMULT.hpp>

#include <fmt/format.h>

namespace Opm {

InjMult::InjMultMode InjMult::injMultModeFromString(const std::string& str, const KeywordLocation& location) {
    if (str == "WREV")
        return InjMultMode::WREV;
    else if (str == "CREV")
        return InjMultMode::CREV;
    else if (str == "CIRR")
        return InjMultMode::CIRR;
    else
        throw OpmInputError(fmt::format("Unknown mode {} is specified in WINJMULT keyword", str), location);
}


bool InjMult::active() const
{
    return this->is_active;
}

bool InjMult::operator==(const InjMult& rhs) const
{
    return is_active == rhs.is_active
        && fracture_pressure == rhs.fracture_pressure
        && multiplier_gradient == rhs.multiplier_gradient;
}


InjMult InjMult::serializationTestObject() {
    InjMult result;
    result.is_active = false;
    result.fracture_pressure = 1.e9;
    result.multiplier_gradient = 2.;
    return result;
}


std::string InjMult::InjMultToString(const InjMult& mult) {
    std::string ss = fmt::format("active? {}, fracture_pressure {}, multiplier_gradient {}",
                                 mult.is_active, mult.fracture_pressure, mult.multiplier_gradient);
    return ss;
}

} // end of namespace Opm