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

#include <cmath>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iomanip>
#include <list>
#include <ostream>
#include <regex>
#include <string>

#include <opm/common/ErrorMacros.hpp>

namespace {

    constexpr std::size_t column_width { 8 } ;
    constexpr std::size_t column_space { 5 } ;

    constexpr std::size_t column_count { 10 } ;

    constexpr std::size_t total_column { column_width + column_space } ;
    constexpr std::size_t total_width  { total_column * column_count } ;

    const std::string version_line { } ;
    // the fact that the dashed header line has 127 rather than 130 dashes has no provenance
    const std::string divider_line { std::string(total_width - 3, '-') } ;

    const std::string block_header_line(const std::string& run_name, const std::string& comment) {
        return "SUMMARY OF RUN " + run_name + " OPM FLOW VERSION 1910 " + comment;
    }

    void write_line(std::ostream& os, const std::string& line, char prefix = ' ') {
        os << prefix << std::setw(total_width) << std::left << line << '\n';
    }

    void print_text_element(std::ostream& os, const std::string& element) {
        os << std::setw(8) << std::left << element << std::setw(5) << "";
    }

    void print_float_element(std::ostream& os, float element) {
        static const std::regex integer_regex { "\\.0*$" };

        auto element_string = std::to_string(element);
        element_string = std::regex_replace(element_string, integer_regex, "");

        if (element_string.size() > 8) {
            element_string = element_string.substr(0, 8);
        }

        os << std::setw(8) << std::right << element_string << std::setw(5) << "";
    }

    void write_header_columns(std::ostream& os, const std::vector<Opm::EclIO::SummaryNode>& vectors, std::function<void(std::ostream&, const Opm::EclIO::SummaryNode&)> print_element, char prefix = ' ') {
        os << prefix;

        for (const auto& vector : vectors) {
            print_element(os, vector);
        }

        os << '\n';
    }

    void write_data_row(std::ostream& os, const std::vector<std::pair<std::vector<float>, int>>& data, std::size_t index, char prefix = ' ') {
        os << prefix;

        for (const auto& vector : data) {
            print_float_element(os, vector.first[index] * std::pow(10.0, -vector.second));
        }

        os << '\n';
    }

    void write_scale_columns(std::ostream& os, const std::vector<std::pair<std::vector<float>, int>> data, char prefix = ' ') {
        os << prefix;

        for (const auto& vector : data) {
            const auto scale_factor { vector.second } ;
            if (scale_factor) {
                print_text_element(os, "*10**" + std::to_string(scale_factor));
            } else {
                print_text_element(os, "");
            }
        }

        os << '\n';
    }

}

namespace Opm::EclIO {

void ESmry::write_block(std::ostream& os, const std::vector<SummaryNode>& vectors) const {
    write_line(os, version_line, '1');
    write_line(os, divider_line);
    write_line(os, block_header_line(inputFileName.stem(), "ANYTHING CAN GO HERE: USER, MACHINE ETC."));
    write_line(os, divider_line);

    std::vector<std::pair<std::vector<float>, int>> data;

    bool has_scale_factors { false } ;
    for (const auto& vector : vectors) {
        int scale_factor { 0 } ;
        const auto& vector_data { get(vector) } ;

        auto max = *std::max_element(vector_data.begin(), vector_data.end());

        if (max >= 10'000'000'000) {
            scale_factor = 6;
            has_scale_factors = true;
        } else if (max >= 10'000'000) {
            scale_factor = 3;
            has_scale_factors = true;
        }

        data.emplace_back(vector_data, scale_factor);
    }

    std::size_t rows { data[0].first.size() };

    write_header_columns(os, vectors, [](std::ostream& os, const SummaryNode& node) { print_text_element(os, node.keyword); });
    write_header_columns(os, vectors, [this](std::ostream& os, const SummaryNode& node) { print_text_element(os, this->get_unit(node)); });
    if (has_scale_factors) {
        write_scale_columns(os, data);
    }
    write_header_columns(os, vectors, [](std::ostream& os, const SummaryNode& node) { print_text_element(os, node.display_name().value_or("")); });
    write_header_columns(os, vectors, [](std::ostream& os, const SummaryNode& node) { print_text_element(os, node.display_number().value_or("")); });

    write_line(os, divider_line);

    for (std::size_t i { 0 } ; i < rows; i++) {
        write_data_row(os, data, i);
    }

    os << std::flush;
}

void ESmry::write_rsm(std::ostream& os) const {
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

    constexpr std::size_t data_column_count { column_count - 1 } ;
    for (std::size_t i { 0 } ; i < data_vectors.size(); i += data_column_count) {
        auto last = std::min(data_vectors.size(), i + data_column_count);
        data_vector_blocks.emplace_back(data_vectors.begin() + i, data_vectors.begin() + last);
        data_vector_blocks.back().push_front(date_vector);
    }

    for (const auto& data_vector_block : data_vector_blocks) {
        write_block(os, { data_vector_block.begin(), data_vector_block.end() });
    }
}

void ESmry::write_rsm_file(std::optional<Opm::filesystem::path> filename) const {
    Opm::filesystem::path summary_file_name { filename.value_or(inputFileName) } ;
    summary_file_name.replace_extension("RSM");

    std::ofstream rsm_file { summary_file_name } ;

    if (!rsm_file.is_open()) {
        OPM_THROW(std::runtime_error, "Could not open file " + std::string(summary_file_name));
    }

    write_rsm(rsm_file);

    rsm_file.close();
}

} // namespace Opm::EclIO
