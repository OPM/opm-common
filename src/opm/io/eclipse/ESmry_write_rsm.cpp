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

#include <opm/io/eclipse/ESmry.hpp>

#include <algorithm>
#include <iomanip>
#include <list>
#include <ostream>
#include <string>

namespace {

constexpr size_t column_width { 8 } ;
constexpr size_t column_space { 5 } ;

constexpr size_t column_count { 10 } ;

constexpr size_t total_column { column_width + column_space } ;
constexpr size_t total_width  { total_column * column_count } ;

const std::string version_line { } ;
// the fact that the dashed header line has 127 rather than 130 dashes has no provenance
const std::string divider_line { std::string(total_width - 3, '-') } ;

const std::string block_header_line(const std::string& run_name, const std::string& comment) {
    return "SUMMARY OF RUN " + run_name + " OPM FLOW VERSION 1910 " + comment;
}

void write_line(std::ostream& os, const std::string& line, char prefix = ' ') {
    os << prefix << std::setw(total_width) << std::left << line << '\n';
}

}

namespace Opm::EclIO {

void ESmry::write_block(std::ostream& os, const std::vector<SummaryNode>& vectors) const {
    write_line(os, version_line, '1');
    write_line(os, divider_line);
    write_line(os, block_header_line(inputFileName.stem(), "ANYTHING CAN GO HERE: USER, MACHINE ETC."));
    write_line(os, divider_line);

    os << ' ';
    for (const auto& vector : vectors) {
        os << std::setw(8) << std::left << vector.keyword << std::setw(5) << "";
    }
    os << '\n';

    os << ' ';
    for (const auto& vector : vectors) {
        os << std::setw(8) << std::left << get_unit(vector) << std::setw(5) << "";
    }
    os << '\n';

    os << ' ';
    for (const auto& vector : vectors) {
        os << std::setw(8) << std::left << vector.display_name().value_or("") << std::setw(5) << "";
    }
    os << '\n';

    os << ' ';
    for (const auto& vector : vectors) {
        os << std::setw(8) << std::left << vector.display_number().value_or("") << std::setw(5) << "";
    }
    os << '\n';

    // write headers

    write_line(os, divider_line);

    // write data
}

void ESmry::write_rsm(std::ostream& os) const {
    os << "Writing for " << summaryNodes.size() << " vectors." << std::endl;

    SummaryNode date_vector;
    std::vector<SummaryNode> data_vectors;
    std::remove_copy_if(summaryNodes.begin(), summaryNodes.end(), std::back_inserter(data_vectors), [&date_vector](const SummaryNode& node){
        if (node.keyword == "TIME" || node.keyword == "DATE") {
            date_vector = node;
            return true;
        } else {
            return false;
        }
    });

    std::vector<std::list<SummaryNode>> data_vector_blocks;

    constexpr size_t data_column_count { column_count - 1 } ;
    for (size_t i { 0 } ; i < data_vectors.size(); i += data_column_count) {
        auto last = std::min(data_vectors.size(), i + data_column_count);
        data_vector_blocks.emplace_back(data_vectors.begin() + i, data_vectors.begin() + last);
        data_vector_blocks.back().push_front(date_vector);
    }

    for (const auto& data_vector_block : data_vector_blocks) {
        write_block(os, { data_vector_block.begin(), data_vector_block.end() });
    }
}

} // namespace Opm::EclIO
