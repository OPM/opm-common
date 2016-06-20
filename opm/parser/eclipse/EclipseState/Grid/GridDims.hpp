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

#ifndef OPM_PARSER_GRIDDIMS_HPP
#define OPM_PARSER_GRIDDIMS_HPP

#include <stdexcept>
#include <array>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

#include <opm/parser/eclipse/Parser/MessageContainer.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/D.hpp> // DIMENS
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp> // SPECGRID

namespace Opm {
    class GridDims
    {
    public:

        GridDims(std::array<int, 3> xyz) :
            GridDims(xyz[0], xyz[1], xyz[2])
        {
        }

        GridDims(size_t nx, size_t ny, size_t nz) :
            m_nx(nx), m_ny(ny), m_nz(nz)
        {
        }

        GridDims(const Deck& deck, MessageContainer messages = MessageContainer())
        {
            std::vector<int> xyz;

            const bool hasRUNSPEC = Section::hasRUNSPEC(deck);
            const bool hasGRID = Section::hasGRID(deck);
            if (hasRUNSPEC && hasGRID) {
                // Equivalent to first constructor.
                RUNSPECSection runspecSection( deck );
                if( runspecSection.hasKeyword<ParserKeywords::DIMENS>() ) {
                    const auto& dimens = runspecSection.getKeyword<ParserKeywords::DIMENS>();
                    xyz = getDims(dimens);
                } else {
                    const std::string msg = "The RUNSPEC section must have the DIMENS keyword with logically Cartesian grid dimensions.";
                    messages.error(msg);
                    throw std::invalid_argument(msg);
                }
            } else if (hasGRID) {
                // Look for SPECGRID instead of DIMENS.
                if (deck.hasKeyword<ParserKeywords::SPECGRID>()) {
                    const auto& specgrid = deck.getKeyword<ParserKeywords::SPECGRID>();
                    xyz = getDims(specgrid);
                } else {
                    const std::string msg = "With no RUNSPEC section, the GRID section must specify the grid dimensions using the SPECGRID keyword.";
                    messages.error(msg);
                    throw std::invalid_argument(msg);
                }
            } else {
                // The deck contains no relevant section, so it is probably a sectionless GRDECL file.
                // Either SPECGRID or DIMENS is OK.
                if (deck.hasKeyword("SPECGRID")) {
                    const auto& specgrid = deck.getKeyword<ParserKeywords::SPECGRID>();
                    xyz = getDims(specgrid);
                } else if (deck.hasKeyword<ParserKeywords::DIMENS>()) {
                    const auto& dimens = deck.getKeyword<ParserKeywords::DIMENS>();
                    xyz = getDims(dimens);
                } else {
                    const std::string msg = "The deck must specify grid dimensions using either DIMENS or SPECGRID.";
                    messages.error(msg);
                    throw std::invalid_argument(msg);
                }
            }

            if (xyz.size() != 3) {
                throw std::invalid_argument("The deck must specify grid dimensions using either DIMENS or SPECGRID.");
            }

            m_nx = xyz[0];
            m_ny = xyz[1];
            m_nz = xyz[2];
        }

        size_t getNX() const {
            return m_nx;
        }

        size_t getNY() const {
            return m_ny;
        }

        size_t getNZ() const {
            return m_nz;
        }

        std::array<int, 3> getNXYZ() const {
            return { {int( m_nx ), int( m_ny ), int( m_nz )}};
        }

        size_t getGlobalIndex(size_t i, size_t j, size_t k) const {
            return (i + j * getNX() + k * getNX() * getNY());
        }

        std::array<int, 3> getIJK(size_t globalIndex) const {
            std::array<int, 3> r = { { 0, 0, 0 } };
            int k = globalIndex / (getNX() * getNY());
            globalIndex -= k * (getNX() * getNY());
            int j = globalIndex / getNX();
            globalIndex -= j * getNX();
            int i = globalIndex;
            r[0] = i;
            r[1] = j;
            r[2] = k;
            return r;
        }

        size_t getCartesianSize() const {
            return m_nx * m_ny * m_nz;
        }

        void assertGlobalIndex(size_t globalIndex) const {
            if (globalIndex >= getCartesianSize())
                throw std::invalid_argument("input index above valid range");
        }

        void assertIJK(size_t i, size_t j, size_t k) const {
            if (i >= getNX() || j >= getNY() || k >= getNZ())
                throw std::invalid_argument("input index above valid range");
        }


    protected:
        GridDims() : m_nx(0), m_ny(0), m_nz(0) {}

        const std::vector<int> getDims() {
            std::vector<int> vec = {static_cast<int>(m_nx),
                                    static_cast<int>(m_ny),
                                    static_cast<int>(m_nz)};
            return vec;
        }

        // keyword must be DIMENS or SPECGRID
        std::vector<int> getDims( const DeckKeyword& keyword ) {
            const auto& record = keyword.getRecord(0);
            std::vector<int> dims = {record.getItem("NX").get< int >(0) ,
                record.getItem("NY").get< int >(0) ,
                record.getItem("NZ").get< int >(0) };
            return dims;
        }

        size_t m_nx;
        size_t m_ny;
        size_t m_nz;
    };
}

#endif /* OPM_PARSER_GRIDDIMS_HPP */
