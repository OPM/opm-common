/*
   Copyright 2020 Equinor ASA.

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

#ifndef OPM_IO_SUMMARYNODE_HPP
#define OPM_IO_SUMMARYNODE_HPP

#include <string>
#include <unordered_set>

namespace Opm::EclIO {

struct SummaryNode {
    enum class Category {
        Aquifer,
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

    std::string keyword;
    Category    category;
    Type        type;
    std::string wgname;
    int         number;

    constexpr static int default_number { std::numeric_limits<int>::min() };

    std::string unique_key() const;
    std::string unique_key(int nI, int nJ, int nK) const;

    bool is_user_defined() const;

    static Category category_from_keyword(const std::string&, const std::unordered_set<std::string> &miscellaneous_keywords = {});
};

} // namespace Opm::EclIO

#endif // OPM_IO_SUMMARYNODE_HPP
