/*
  Copyright 2016  Statoil ASA.

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

#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/io/eclipse/EGrid.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp> // DIMENS
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp> // GDFILE
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp> // SPECGRID

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <array>
#include <stdexcept>
#include <vector>

namespace Opm {

    GridDims::GridDims()
        : m_nx(0), m_ny(0), m_nz(0)
    {}

    GridDims::GridDims(const std::array<int, 3>& xyz)
        : GridDims(xyz[0], xyz[1], xyz[2])
    {}

    GridDims::GridDims(const std::size_t nx,
                       const std::size_t ny,
                       const std::size_t nz)
        : m_nx(nx), m_ny(ny), m_nz(nz)
    {}

    GridDims GridDims::serializationTestObject()
    {
        return { 1, 2, 3 };
    }

    GridDims::GridDims(const Deck& deck)
    {
        if (deck.hasKeyword<ParserKeywords::SPECGRID>()) {
            this->init(deck[ParserKeywords::SPECGRID::keywordName].back());
        }
        else if (deck.hasKeyword<ParserKeywords::DIMENS>()) {
            this->init(deck[ParserKeywords::DIMENS::keywordName].back());
        }
        else if (deck.hasKeyword<ParserKeywords::GDFILE>()) {
            this->binary_init(deck);
        }
        else {
            throw std::invalid_argument {
                "Must have either SPECGRID or DIMENS "
                "to indicate grid dimensions"
            };
        }
    }

    std::size_t GridDims::getNX() const { return this->m_nx; }
    std::size_t GridDims::getNY() const { return this->m_ny; }
    std::size_t GridDims::getNZ() const { return this->m_nz; }

    std::size_t GridDims::operator[](int dim) const
    {
        switch (dim) {
        case 0: return this->getNX();
        case 1: return this->getNY();
        case 2: return this->getNZ();

        default:
            throw std::invalid_argument("Invalid argument dim:" + std::to_string(dim));
        }
    }

    std::array<int, 3> GridDims::getNXYZ() const
    {
        return {
            static_cast<int>(this->getNX()),
            static_cast<int>(this->getNY()),
            static_cast<int>(this->getNZ())
        };
    }

    std::size_t
    GridDims::getGlobalIndex(const std::size_t i,
                             const std::size_t j,
                             const std::size_t k) const
    {
        return i + this->getNX()*(j + k*this->getNY());
    }

    std::array<int, 3>
    GridDims::getIJK(std::size_t globalIndex) const
    {
        auto ijk = std::array<int, 3>{};

        ijk[0] = globalIndex % this->getNX();  globalIndex /= this->getNX();
        ijk[1] = globalIndex % this->getNY();  globalIndex /= this->getNY();
        ijk[2] = globalIndex;

        return ijk;
    }

    std::size_t GridDims::getCartesianSize() const
    {
        return m_nx * m_ny * m_nz;
    }

    void GridDims::assertGlobalIndex(std::size_t globalIndex) const
    {
        if (globalIndex >= getCartesianSize())
            throw std::invalid_argument("input global index above valid range");
    }

    void GridDims::assertIJK(const std::size_t i,
                             const std::size_t j,
                             const std::size_t k) const
    {
        if (i >= getNX() || j >= getNY() || k >= getNZ())
            throw std::invalid_argument("input IJK index above valid range");
    }

    // keyword must be DIMENS or SPECGRID
    inline std::array<int, 3> readDims(const DeckKeyword& keyword)
    {
        const auto& record = keyword.getRecord(0);
        return {
            record.getItem("NX").get<int>(0),
            record.getItem("NY").get<int>(0),
            record.getItem("NZ").get<int>(0)
        };
    }

    void GridDims::init(const DeckKeyword& keyword)
    {
        const auto dims = readDims(keyword);
        m_nx = dims[0];
        m_ny = dims[1];
        m_nz = dims[2];
    }

    void GridDims::binary_init(const Deck& deck)
    {
        const DeckKeyword& gdfile_kw = deck["GDFILE"].back();
        const std::string& gdfile_arg = gdfile_kw.getRecord(0).getItem("filename").get<std::string>(0);
        const EclIO::EGrid egrid( deck.makeDeckPath(gdfile_arg) );

        const auto& dimens = egrid.dimension();
        m_nx = dimens[0];
        m_ny = dimens[1];
        m_nz = dimens[2];
    }

    bool GridDims::operator==(const GridDims& data) const
    {
        return this->getNXYZ() == data.getNXYZ();
    }

}
