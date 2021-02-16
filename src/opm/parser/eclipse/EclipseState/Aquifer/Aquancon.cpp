/*
  Copyright (C) 2017 TNO

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
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/Aquancon.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <fmt/format.h>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <iterator>
#include <iostream>

#include "AquiferHelpers.hpp"

namespace Opm {

    namespace{

        double face_area(FaceDir::DirEnum face_dir, std::size_t global_index, const EclipseGrid& grid) {
            const auto& dims = grid.getCellDims(global_index);
            switch (face_dir) {
            case FaceDir::XPlus:
            case FaceDir::XMinus:
                return dims[1] * dims[2];
            case FaceDir::YPlus:
            case FaceDir::YMinus:
                return dims[0] * dims[2];
            case FaceDir::ZPlus:
            case FaceDir::ZMinus:
                return dims[0] * dims[1];
            default:
                throw std::logic_error("What the f...");
            }
        }

        void add_cell(const KeywordLocation& location,
                      std::unordered_map<std::size_t, Aquancon::AquancCell>& work,
                      const EclipseGrid& grid,
                      int aquiferID,
                      std::size_t global_index,
                      std::optional<double> influx_coeff,
                      double influx_mult,
                      FaceDir::DirEnum face_dir) {
            auto cell_iter = work.find(global_index);
            if (cell_iter == work.end()) {
                if (!influx_coeff.has_value())
                    influx_coeff = face_area(face_dir, global_index, grid);
                work.insert( std::make_pair(global_index, Aquancon::AquancCell(aquiferID, global_index, influx_coeff.value() * influx_mult, face_dir)) );
            }
            else {
                auto& prev_cell = cell_iter->second;
                if (prev_cell.aquiferID == aquiferID) {
                    prev_cell.influx_coeff += influx_coeff.value_or(0.0);
                    prev_cell.influx_coeff *= influx_mult;
                } else {
                    auto [i,j,k] = grid.getIJK(global_index);
                    auto msg = fmt::format("Problem with AQUANCON keyword\n"
                                           "In {} line {}\n"
                                           "Cell ({}, {}, {}) is already connected to aquifer: {}", location.filename, location.lineno, i + 1, j + 1, k + 1, prev_cell.aquiferID);
                    throw std::invalid_argument( msg );
                }
            }
        }

    }


    Aquancon::Aquancon(const EclipseGrid& grid, const Deck& deck)
    {
        std::unordered_map<std::size_t, Aquancon::AquancCell> work;
        for (std::size_t iaq = 0; iaq < deck.count("AQUANCON"); iaq++) {
            const auto& aquanconKeyword = deck.getKeyword("AQUANCON", iaq);
            OpmLog::info(OpmInputError::format("Initializing aquifer connections from {keyword} in {file} line {line}", aquanconKeyword.location()));
            for (const auto& aquanconRecord : aquanconKeyword) {
                const int aquiferID = aquanconRecord.getItem("AQUIFER_ID").get<int>(0);
                const int i1 = aquanconRecord.getItem("I1").get<int>(0) - 1;
                const int i2 = aquanconRecord.getItem("I2").get<int>(0) - 1;
                const int j1 = aquanconRecord.getItem("J1").get<int>(0) - 1;
                const int j2 = aquanconRecord.getItem("J2").get<int>(0) - 1;
                const int k1 = aquanconRecord.getItem("K1").get<int>(0) - 1;
                const int k2 = aquanconRecord.getItem("K2").get<int>(0) - 1;
                const double influx_mult = aquanconRecord.getItem("INFLUX_MULT").getSIDouble(0);
                const FaceDir::DirEnum faceDir
                    = FaceDir::FromString(aquanconRecord.getItem("FACE").getTrimmedString(0));

                const std::string& str_inside_reservoir
                    = aquanconRecord.getItem("CONNECT_ADJOINING_ACTIVE_CELL").getTrimmedString(0);
                const bool allow_aquifer_inside_reservoir = DeckItem::to_bool(str_inside_reservoir);

                // Loop over the cartesian indices to convert to the global grid index
                for (int k = k1; k <= k2; k++) {
                    for (int j = j1; j <= j2; j++) {
                        for (int i = i1; i <= i2; i++) {
                            if (grid.cellActive(i, j, k)) { // the cell itself needs to be active
                                if (allow_aquifer_inside_reservoir
                                    || !AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, faceDir)) {
                                    std::optional<double> influx_coeff;
                                    if (aquanconRecord.getItem("INFLUX_COEFF").hasValue(0))
                                        influx_coeff = aquanconRecord.getItem("INFLUX_COEFF").getSIDouble(0);

                                    auto global_index = grid.getGlobalIndex(i,j,k);
                                    add_cell(aquanconKeyword.location(), work, grid, aquiferID, global_index, influx_coeff, influx_mult, faceDir);
                                }
                            }
                        }
                    }
                }
            }
        }

        for (const auto& gi_cell : work) {
            const auto& cell = gi_cell.second;

            this->cells[cell.aquiferID].emplace_back(std::move(cell));
        }
    }


    Aquancon Aquancon::serializeObject()
    {
        Aquancon result;
        result.cells = {{1, {{2, 3, 4.0, FaceDir::XPlus}}}};

        return result;
    }


    const std::vector<Aquancon::AquancCell> Aquancon::operator[](int aquiferID) const {
        return this->cells.at( aquiferID );
    }

    Aquancon::Aquancon(const std::unordered_map<int, std::vector<Aquancon::AquancCell>>& data) :
        cells(data)
    {}

    const std::unordered_map<int, std::vector<Aquancon::AquancCell>>& Aquancon::data() const {
        return this->cells;
    }

    bool Aquancon::operator==(const Aquancon& other) const {
        return this->cells == other.cells;
    }


    bool Aquancon::active() const {
        return !this->cells.empty();
    }

}
