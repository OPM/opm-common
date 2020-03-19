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

#include <numeric>
#include <vector>

#include <opm/io/eclipse/SummaryNode.hpp>

namespace {

constexpr bool use_number(Opm::EclIO::SummaryNode::Category category) {
    switch (category) {
    case Opm::EclIO::SummaryNode::Category::Block:      [[fallthrough]];
    case Opm::EclIO::SummaryNode::Category::Connection: [[fallthrough]];
    case Opm::EclIO::SummaryNode::Category::Region:     [[fallthrough]];
    case Opm::EclIO::SummaryNode::Category::Segment:
        return true;
    default:
        return false;
    }
}

constexpr bool use_name(Opm::EclIO::SummaryNode::Category category) {
    switch (category) {
    case Opm::EclIO::SummaryNode::Category::Connection: [[fallthrough]];
    case Opm::EclIO::SummaryNode::Category::Group:      [[fallthrough]];
    case Opm::EclIO::SummaryNode::Category::Segment:    [[fallthrough]];
    case Opm::EclIO::SummaryNode::Category::Well:
        return true;
    default:
        return false;
    }
}

};

std::string Opm::EclIO::SummaryNode::unique_key() const {
    std::vector<std::string> key_parts { keyword } ;

    if (use_number(category))
        key_parts.emplace_back(std::to_string(number));

    if (use_name(category))
        key_parts.emplace_back(name);

    auto compose_key = [](std::string& key, const std::string& key_part) -> std::string {
        constexpr auto delimiter { ':' } ;
        return key.empty() ? key_part : key + delimiter + key_part;
    };

    return std::accumulate(std::begin(key_parts), std::end(key_parts), std::string(), compose_key);
}
