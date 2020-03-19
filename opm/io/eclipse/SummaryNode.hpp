/*
   Copyright 2020 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include <string>

namespace Opm::EclIO {

struct SummaryNode {
    enum class Category {
        Well,
        Group,
        Field,
        Region,
        Block,
        Connection,
        Segment,
        Miscellaneous,
    };

    enum class Type {
        Rate,
        Total,
        Ratio,
        Pressure,
        Count,
        Mode,
        Undefined,
    };

    const std::string keyword;
    const Category category;
    const Type type;
    const std::string wgname;
    const int number;

    std::string unique_key() const;
};

} // namespace Opm::EclIO
