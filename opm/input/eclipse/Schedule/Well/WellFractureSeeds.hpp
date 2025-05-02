/*
  Copyright 2025 Equinor ASA.

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
#ifndef OPM_WELL_FRACTURE_SEED_HPP_INCLUDED
#define OPM_WELL_FRACTURE_SEED_HPP_INCLUDED

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace Opm {

/// Fracture seed points attached to a single well.
class WellFractureSeeds
{
public:
    /// Disambiguating type for requesting fracture plane normal vectors
    /// based on insertion indices.
    struct SeedIndex
    {
        /// Insertion/record index.
        std::size_t i{};
    };

    /// Disambiguating type for requesting fracture plane normal vectors
    /// based on Cartesian cell indices.
    struct SeedCell
    {
        /// Cartesian cell index.
        std::size_t c{};
    };

    /// Type alias for the normal vector at a single seed point.
    using NormalVector = std::array<double, 3>;

    /// Default constructor.
    ///
    /// Forms an object which is mostly usable as the destination of a
    /// deserialisation operation.
    WellFractureSeeds() = default;

    /// Constructor.
    ///
    /// \param[in] wellName Named well to which this seed collection is
    /// associated.
    explicit WellFractureSeeds(const std::string& wellName)
        : wellName_ { wellName }
    {}

    /// Named well to which this seed collection is associated.
    ///
    /// Exists mostly to meet interface requirements of class
    /// ScheduleState::map_member<>.
    const std::string& name() const
    {
        return this->wellName_;
    }

    /// Insert or update a fracture seed in current collection.
    ///
    /// \param[in] seedCellGlobal Linearised Cartesian cell index.  Should
    /// typically correspond to a reservoir connection for the named well.
    ///
    /// \param[in] seedNormal Fracturing plane's normal vector.  Need not be
    /// a unit normal as far as class WellFractureSeeds goes, but subsequent
    /// uses may prefer unit normals.
    ///
    /// \return Whether or not a seed was inserted/updated.  Typically
    /// 'true'.
    bool updateSeed(const std::size_t   seedCellGlobal,
                    const NormalVector& seedNormal);

    /// Establish accelerator structure for LOG(n) normal vector lookup
    /// based on Cartesian cell indices.
    ///
    /// This is an optimisation that requires more memory in the object, and
    /// you should call this function only when all updateSeed() calls have
    /// been made.  You do not need to call this function in order to use
    /// the object, but it will reduce the cost of those kinds of lookup.
    /// If you do not call this function, then normal vector lookup based on
    /// Cartesian cell indices will use a linear search.
    void finalizeSeeds();

    /// Predicate for empty fracture seed collection.
    ///
    /// \return Whether or not the current collection is empty.
    bool empty() const { return this->seedCell_.empty(); }

    /// Number of fracture seeds in the current collection.
    auto numSeeds() const { return this->seedCell_.size(); }

    /// Look up fracturing plane normal vector based on Cartesian cell index.
    ///
    /// \param[in] c Cartesian cell index.
    ///
    /// \return Fracturing plane normal vector in cell \p c.  Not guaranteed
    /// to be a unit normal vector.  Nullptr if no seed exists in cell \p c.
    const NormalVector* getNormal(const SeedCell& c) const;

    /// Retrieve fracturing plane normal vector based on insertion
    /// order/record index.
    ///
    /// Should normally be used in conjunction with member function
    /// seedCells() only.
    ///
    /// \param[in] i Insertion order.  Should be in the range [0
    /// .. numSeeds()).
    ///
    /// \return Fracturing plane normal vector in seed cell inserted as the
    /// \p i-th unique cell index.  Not guaranteed to be a unit normal vector.
    const NormalVector& getNormal(const SeedIndex& i) const
    {
        return this->seedNormal_[i.i];
    }

    /// Retrieve this collection's fracture seed cells
    ///
    /// \return Sequence of Cartesian cell indices.  The normal vector of
    /// the fracturing plane in cell \code seedCells()[i] \endcode is \code
    /// getNormal(SeedIndex{i}) \endcode.
    const std::vector<std::size_t>& seedCells() const
    {
        return this->seedCell_;
    }

    /// Equality predicate.
    ///
    /// \param[in] that Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p that.
    bool operator==(const WellFractureSeeds& that) const;

    /// Create a serialisation test object.
    static WellFractureSeeds serializationTestObject();

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->wellName_);
        serializer(this->seedCell_);
        serializer(this->seedNormal_);
        serializer(this->lookup_);
    }

private:
    using NormalVectorIx = std::vector<NormalVector>::size_type;

    /// Named well to which this fracture seed collection is associated.
    ///
    /// Mostly exists to meet the interface requirement of class
    /// ScheduleState::map_member<>.
    std::string wellName_{};

    /// Cartesian indices in insertion order of this collection fracture
    /// seed cells.
    std::vector<std::size_t> seedCell_{};

    /// Fracturing plane normal vectors for all seed cells.
    std::vector<NormalVector> seedNormal_{};

    /// Binary search lookup structure.
    ///
    /// Indices into seedCell_ and seedNormal_ ordered by seedCell_ values.
    std::vector<NormalVectorIx> lookup_{};

    /// Back end for finalizeSeeds().
    ///
    /// Builds the lookup_ array to enable using binary search for Cartesian
    /// cells.
    void establishLookup();

    /// Compute insertion order index for Cartesian cell index.
    ///
    /// Switches between linear and binary search based on availability of
    /// lookup_ data.
    ///
    /// \param[in] seedCellGlobal Linearised Cartesian cell index.
    ///
    /// \return Insertion order index for \p seedCellGlobal.  Returns
    /// numSeeds() if \p seedCellGlobal does not exist in seedCell_.
    NormalVectorIx seedIndex(const std::size_t seedCellGlobal) const;

    /// Compute insertion order index for Cartesian cell index using binary
    /// search.
    ///
    /// \param[in] seedCellGlobal Linearised Cartesian cell index.
    ///
    /// \return Insertion order index for \p seedCellGlobal.  Returns
    /// numSeeds() if \p seedCellGlobal does not exist in seedCell_.
    NormalVectorIx seedIndexBinarySearch(const std::size_t seedCellGlobal) const;

    /// Compute insertion order index for Cartesian cell index using linear
    /// search.
    ///
    /// \param[in] seedCellGlobal Linearised Cartesian cell index.
    ///
    /// \return Insertion order index for \p seedCellGlobal.  Returns
    /// numSeeds() if \p seedCellGlobal does not exist in seedCell_.
    NormalVectorIx seedIndexLinearSearch(const std::size_t seedCellGlobal) const;

    /// Insert a new seed cell and associate normal vector into collection.
    ///
    /// Invalidates lookup_ data.
    ///
    /// \param[in] Linearised Cartesian cell index
    ///
    /// \param[in] seedNormal Fracturing plane's normal vector.
    ///
    /// \return Whether or not a seed was inserted/updated.  Typically
    /// 'true'.
    bool insertNewSeed(const std::size_t seedCellGlobal, const NormalVector& seedNormal);

    /// Update normal vector direction of an existing seed cell.
    ///
    /// \param[in] ix Insertion order index for an existing seed cell.
    /// Typically computed by seedIndex().
    ///
    /// \param[in] seedNormal Fracturing plane's normal vector.
    ///
    /// \return Whether or not the normal vector for seed \p ix was updated.
    bool updateExistingSeed(const NormalVectorIx ix, const NormalVector& seedNormal);
};

} // namespace Opm

#endif // OPM_WELL_FRACTURE_SEED_HPP_INCLUDED
