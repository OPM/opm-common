/*
  Copyright 2021 Equinor ASA.

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

#ifndef COMPLETED_CELLS
#define COMPLETED_CELLS

#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>

namespace Opm {

/// Sparse collection of cells, and their properties, intersected by one or
/// more well connections.
class CompletedCells
{
public:
    /// Identification and associate properties of cell intersected by one
    /// or more well connections.
    struct Cell
    {
        /// Property data of intersected cell
        struct Props
        {
            /// Cell's active index in the range [0 .. #active).
            ///
            /// Relative to current grid--e.g., an LGR or the main grid.
            std::size_t active_index{};

            /// Cell's permeability component in the grid's X direction.
            double permx{};

            /// Cell's permeability component in the grid's Y direction.
            double permy{};

            /// Cell's permeability component in the grid's Z direction.
            double permz{};

            /// Cell's porosity.
            double poro{};

            /// Cell's net-to-gross ratio.
            double ntg{};

            /// Cell's saturation region.
            int satnum{};

            /// Cell's PVT region index.
            int pvtnum{};

            /// Equality predicate.
            ///
            /// \param[in] other Object against which \code *this \endcode
            /// will be tested for equality.
            ///
            /// \return Whether or not \code *this \endcode is the same as
            /// \p other.
            bool operator==(const Props& other) const;

            /// Create a serialisation test object.
            static Props serializationTestObject();

            /// Convert between byte array and object representation.
            ///
            /// \tparam Serializer Byte array conversion protocol.
            ///
            /// \param[in,out] serializer Byte array conversion object.
            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->active_index);
                serializer(this->permx);
                serializer(this->permy);
                serializer(this->permz);
                serializer(this->poro);
                serializer(this->satnum);
                serializer(this->pvtnum);
                serializer(this->ntg);
            }
        };

        /// Default constructor.
        ///
        /// Creates a Cell object that's mostly usable as the target of a
        /// deserialisation operation.
        Cell() = default;

        /// Constructor
        ///
        /// \param[in] g Cell's linearised Cartesian index relative to
        /// grid's origin.
        ///
        /// \param[in] i_ Cell's Cartesian I index relative to grid's
        /// origin.
        ///
        /// \param[in] j_ Cell's Cartesian J index relative to grid's
        /// origin.
        ///
        /// \param[in] k_ Cell's Cartesian K index relative to grid's
        /// origin.
        Cell(const std::size_t g,
             const std::size_t i_,
             const std::size_t j_,
             const std::size_t k_)
            : global_index(g)
            , i(i_)
            , j(j_)
            , k(k_)
        {}

        /// Linearised Cartesian cell index
        ///
        /// Relative to grid origin--e.g., in an LGR or in the main grid.
        std::size_t global_index{};

        /// Cartesian I index relative to grid origin.
        std::size_t i{};

        /// Cartesian J index relative to grid origin.
        std::size_t j{};

        /// Cartesian K index relative to grid origin.
        std::size_t k{};

        /// Depth of cell centre.
        double depth{};

        /// Physical cell extents.
        std::array<double, 3> dimensions{};

        /// Cell property data.
        ///
        /// Nullopt if cell has not yet been discovered.
        std::optional<Props> props{};

        /// Check if cell is discovered and has associated property data.
        bool is_active() const;

        /// Retrieve cell's active index grid.
        ///
        /// Will throw an exception unless cell is_active().
        std::size_t active_index() const;

        /// Equality predicate.
        ///
        /// \param[in] other Object against which \code *this \endcode will be
        /// tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// other.
        bool operator==(const Cell& other) const;

        /// Create a serialisation test object.
        static Cell serializationTestObject();

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->global_index);
            serializer(this->i);
            serializer(this->j);
            serializer(this->k);
            serializer(this->props);
            serializer(this->depth);
            serializer(this->dimensions);
        }
    };

    /// Default constructor.
    ///
    /// Creates a collection that is only usable as the target of a
    /// deserialisation operation.
    CompletedCells() = default;

    /// Constructor.
    ///
    /// \param[in] dims Host grid's Cartesian dimensions.  Needed to
    /// translate between linearised Cartesian indices and (I,J,K) tuples.
    explicit CompletedCells(const GridDims& dims);

    /// Constructor.
    ///
    /// \param[in] nx Host grid's Cartesian dimension in the X direction.
    /// \param[in] ny Host grid's Cartesian dimension in the Y direction.
    /// \param[in] nz Host grid's Cartesian dimension in the Z direction.
    CompletedCells(std::size_t nx, std::size_t ny, std::size_t nz);

    /// Retrieve intersected cell.
    ///
    /// Will throw an exception if the cell does not exist in the current
    /// collection.
    ///
    /// \param[in] i Cell's Cartesian I index relative to grid's origin.
    /// \param[in] j Cell's Cartesian J index relative to grid's origin.
    /// \param[in] k Cell's Cartesian K index relative to grid's origin.
    ///
    /// \return Intersected cell at specified coordinates.
    const Cell& get(std::size_t i, std::size_t j, std::size_t k) const;

    /// Retrieve, and possibly create, an intersected cell.
    ///
    /// Will insert a cell object into the collection if none exist at
    /// specified coordinate.
    ///
    /// \param[in] i Cell's Cartesian I index relative to grid's origin.
    /// \param[in] j Cell's Cartesian J index relative to grid's origin.
    /// \param[in] k Cell's Cartesian K index relative to grid's origin.
    ///
    /// \return Cell object and existence status.  The existence status is
    /// 'false' if a new cell object was inserted into the collection as a
    /// result of this request and 'true' otherwise.
    std::pair<Cell*, bool>
    try_get(std::size_t i, std::size_t j, std::size_t k);

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p other.
    bool operator==(const CompletedCells& other) const;

    /// Create a serialisation test object.
    static CompletedCells serializationTestObject();

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->dims);
        serializer(this->cells);
    }

private:
    /// Host grid's Cartesian dimensions.
    GridDims dims;

    /// Sparse collection of intersected cells.
    ///
    /// Keyed by linearised Cartesian index.
    std::unordered_map<std::size_t, Cell> cells{};
};

} // namespace Opm

#endif  // COMPLETED_CELLS
