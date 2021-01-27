/*
  Copyright (C) 2020 SINTEF Digital

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

#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferConnection.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferCell.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/SingleNumericalAquifer.hpp>

namespace Opm {
    SingleNumericalAquifer::SingleNumericalAquifer(const size_t aqu_id)
            : id_(aqu_id)
    {
    }

    void SingleNumericalAquifer::addAquiferCell(const NumericalAquiferCell& aqu_cell) {
        cells_.push_back(aqu_cell);
    }

    size_t SingleNumericalAquifer::numCells() const {
        return this->cells_.size();
    }

    const NumericalAquiferCell* SingleNumericalAquifer::getCellPrt(const size_t index) const {
        return &this->cells_[index];
    }

    void SingleNumericalAquifer::addAquiferConnection(const NumericalAquiferConnection& aqu_con) {
        this->connections_.push_back(aqu_con);
    }

    bool SingleNumericalAquifer::operator==(const SingleNumericalAquifer& other) const {
        return this->cells_ == other.cells_ &&
                this->connections_ == other.connections_ &&
                this->id_ == other.id_;
    }

    size_t SingleNumericalAquifer::numConnections() const {
        return this->connections_.size();
    }
}
