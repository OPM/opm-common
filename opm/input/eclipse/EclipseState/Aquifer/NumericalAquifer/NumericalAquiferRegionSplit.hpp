/*
  Copyright 2026 Equinor ASA.

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

#ifndef OPM_NUMERICAL_AQUIFER_REGION_SPLIT_HPP
#define OPM_NUMERICAL_AQUIFER_REGION_SPLIT_HPP

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace Opm {

class Aqudims;
class EclipseGrid;
class FieldPropsManager;
class LgrCollection;
class NumericalAquifers;
class UnitSystem;

/// Deck changes which place numerical aquifers in a single equilibration region.
///
/// A numerical aquifer whose cells and connected reservoir cells are not all
/// initialised from the same equilibration region (EQLNUM) is in general not
/// in hydrostatic equilibrium at the start of the simulation, and fluid
/// flows through the aquifer between the regions.  For each of the given
/// aquifers this class computes the AQUNUM, AQUCON and EQUALS records which
///
///   - split the aquifer into one aquifer per EQLNUM region of its connected
///     cells.  The original aquifer keeps the connections to the region of
///     its own cells if possible, otherwise those of the region with most
///     connections.  New aquifers are copies of the original one placed in
///     free, inactive cells outside local grid refinements, with the depth
///     of the original aquifer cells.  Connections transferred to a new
///     aquifer through a face adjoining a cell of the original aquifer are
///     declared with ALLOW_INTERNAL_CELLS='YES', since that cell is not
///     part of the new aquifer.
///   - put the cells of each aquifer in the EQLNUM region of its connected
///     cells, and use the PVT table of most of these cells.
///   - give the cells of new aquifers the cell properties of the original
///     aquifer cells they are copied from: GRID (NTG, thermal properties,
///     MULTNUM, ...), PROPS (saturation function end-points, SWATINIT,
///     PCW, ...) and REGIONS (FIPNUM, ROCKNUM, ...) section properties.
///     The properties a free, inactive cell gets from the deck are often
///     meaningless, e.g. NTG=0 or SWU=0, and may make the initialisation
///     or the simulation fail.  Properties defined by AQUNUM, and
///     transmissibility and pore volume multipliers, are not copied.
///
/// AQUCON records are only generated for aquifers which are split, and
/// AQUNUM records only for new aquifers and for aquifers whose PVT table or
/// initial pressure changes.  An explicit initial pressure is replaced by
/// the default (equilibrated) value.
class NumericalAquiferRegionSplit
{
public:
    struct AqunumRecord
    {
        std::size_t id{};        //!< Aquifer ID
        std::size_t sourceId{};  //!< Aquifer ID the record is derived from
        std::size_t globalIndex{};
        std::array<std::size_t, 3> ijk{}; //!< Zero-based cell indices
        double area{};           //!< SI units
        double length{};
        double porosity{};
        double permeability{};
        double depth{};
        int pvtTable{};
        int satTable{};
        int region{};            //!< EQLNUM region of the connected cells
        bool hadInitialPressure{false};
    };

    struct AquconRecord
    {
        std::size_t id{};
        std::array<std::size_t, 6> box{}; //!< Zero-based I1, I2, J1, J2, K1, K2
        FaceDir::DirEnum face{FaceDir::Unknown};
        double transMult{1.0};
        int transOption{0};
        bool allowInternalCells{false};
        double veFrac{1.0};
        double veFracP{1.0};
    };

    struct EqlnumRecord
    {
        int region{};
        std::size_t aquiferId{};
        std::array<std::size_t, 3> ijk{}; //!< Zero-based cell indices
    };

    /// Cell property of a new aquifer cell, copied from the original
    /// aquifer cell.
    struct PropertyRecord
    {
        std::string keyword{};
        std::size_t aquiferId{};
        std::array<std::size_t, 3> ijk{}; //!< Zero-based cell indices
        double value{};                   //!< SI units
        std::string dimension{};          //!< Unit of value, empty if dimensionless
        bool isInteger{false};
    };

    /// \param aquifers Numerical aquifers, with connections processed
    ///                 against the final set of active cells.
    /// \param aquiferIds Aquifers for which deck changes are requested.
    /// \param grid Input grid.
    /// \param fieldProps Global field properties (EQLNUM, PVTNUM and the
    ///                   properties of aquifer cells).
    /// \param lgrs Local grid refinements, which may not contain aquifer cells.
    NumericalAquiferRegionSplit(const NumericalAquifers& aquifers,
                                const std::vector<std::size_t>& aquiferIds,
                                const EclipseGrid& grid,
                                const FieldPropsManager& fieldProps,
                                const LgrCollection& lgrs);

    bool empty() const;

    /// AQUNUM records replacing those of replacedAqunumIds() and adding new
    /// aquifers.
    const std::vector<AqunumRecord>& aqunumRecords() const
    { return this->aqunum_; }

    /// AQUCON records replacing all records of replacedAquconIds().
    const std::vector<AquconRecord>& aquconRecords() const
    { return this->aqucon_; }

    /// EQLNUM values for aquifer cells.
    const std::vector<EqlnumRecord>& eqlnumRecords() const
    { return this->eqlnum_; }

    /// GRID section properties of new aquifer cells.
    const std::vector<PropertyRecord>& gridRecords() const
    { return this->grid_; }

    /// PROPS section properties of new aquifer cells.
    const std::vector<PropertyRecord>& propsRecords() const
    { return this->props_; }

    /// REGIONS section properties, other than EQLNUM, PVTNUM and SATNUM,
    /// of new aquifer cells.
    const std::vector<PropertyRecord>& regionsRecords() const
    { return this->regions_; }

    /// Number of AQUNUM records (aquifer cells) after the deck changes.
    std::size_t numAqunumRecords() const
    { return this->numAqunumRecords_; }

    /// Number of AQUCON records after the deck changes.
    std::size_t numAquconRecords() const
    { return this->numAquconRecords_; }

    const std::vector<std::size_t>& replacedAqunumIds() const
    { return this->replacedAqunum_; }

    const std::vector<std::size_t>& replacedAquconIds() const
    { return this->replacedAqucon_; }

    /// Aquifers for which no free inactive cell outside local grid
    /// refinements could be found for the new aquifers.  No records are
    /// generated for these.
    const std::vector<std::size_t>& unresolvedIds() const
    { return this->unresolved_; }

    /// Human readable description of the deck changes in deck units.
    std::string deckChanges(const UnitSystem& units) const;

    /// Human readable description of the deck changes in deck units,
    /// including an AQUDIMS record if the given dimensions do not allow
    /// for the AQUNUM and AQUCON records after the changes.
    std::string deckChanges(const UnitSystem& units, const Aqudims& aqudims) const;

private:
    std::string deckChangesImpl(const UnitSystem& units, const Aqudims* aqudims) const;

    std::vector<AqunumRecord> aqunum_{};
    std::vector<AquconRecord> aqucon_{};
    std::vector<EqlnumRecord> eqlnum_{};
    std::vector<PropertyRecord> grid_{};
    std::vector<PropertyRecord> props_{};
    std::vector<PropertyRecord> regions_{};
    std::size_t numAqunumRecords_{0};
    std::size_t numAquconRecords_{0};
    std::vector<std::size_t> replacedAqunum_{};
    std::vector<std::size_t> replacedAqucon_{};
    std::vector<std::size_t> unresolved_{};
};

} // namespace Opm

#endif // OPM_NUMERICAL_AQUIFER_REGION_SPLIT_HPP
