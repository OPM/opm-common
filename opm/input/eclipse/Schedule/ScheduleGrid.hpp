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
#ifndef SCHEDULE_GRID
#define SCHEDULE_GRID

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

class EclipseGrid;
class FieldPropsManager;
class NumericalAquifers;
struct NumericalAquiferCell;

} // namespace Opm

namespace Opm {

/// Collection of intersected cells and associate properties for all
/// simulation grids, i.e., the main grid and all LGRs in the simulation
/// run.
///
/// Holds references to mutable collections of CompletedCells and will
/// populate these as needed.  Those collections must outlive the
/// ScheduleGrid object.
class ScheduleGrid
{
public:
    /// Constructor.
    ///
    /// Applies to main grid only.  Will not be able to create new cell
    /// objects even if such objects are needed.
    ///
    /// \param[in,out] completed_cells Intersected cells and associate
    /// properties.  Reference to a mutable collection which must outlive
    /// the ScheduleGrid object.
    explicit ScheduleGrid(CompletedCells& completed_cells);

    /// Constructor.
    ///
    /// Applies to main grid and any LGRs.  Will not be able to create new
    /// cell objects even if such objects are needed.
    ///
    /// \param[in,out] completed_cells Intersected cells and associate
    /// properties.  Reference to a mutable collection which must outlive
    /// the ScheduleGrid object.
    ///
    /// \param[in,out] completed_cells_lgr Intersected cells and associate
    /// properties for all LGRs.  Reference to a mutable collection which
    /// must outlive the ScheduleGrid object.  Empty if there are no LGRs.
    ///
    /// \param[in] label_to_index_ Translation table from LGR names to LGR
    /// indices.  Empty if there are no LGRs.
    ScheduleGrid(CompletedCells&              completed_cells,
                 std::vector<CompletedCells>& completed_cells_lgr,
                 const std::unordered_map<std::string, std::size_t>& label_to_index_);

    /// Constructor.
    ///
    /// Will populate collection of completed cells if needed.
    ///
    /// \param[in] ecl_grid Grid object with which to associate intersected
    /// cells.
    ///
    /// \param[in] fpm Property container from which to extract property
    /// data of intersected cells.
    ///
    /// \param[in,out] completed_cells Intersected cells and associate
    /// properties.  Reference to a mutable collection which must outlive
    /// the ScheduleGrid object.
    ScheduleGrid(const EclipseGrid&       ecl_grid,
                 const FieldPropsManager& fpm,
                 CompletedCells&          completed_cells);

    /// Constructor.
    ///
    /// Will populate collection of completed cells if needed.
    ///
    /// \param[in] ecl_grid Grid object with which to associate intersected
    /// cells.
    ///
    /// \param[in] fpm Property container from which to extract property
    /// data of intersected cells.
    ///
    /// \param[in,out] completed_cells Intersected cells and associate
    /// properties.  Reference to a mutable collection which must outlive
    /// the ScheduleGrid object.
    ///
    /// \param[in,out] completed_cells_lgr Intersected cells and associate
    /// properties for all LGRs.  Reference to a mutable collection which
    /// must outlive the ScheduleGrid object.  Empty if there are no LGRs.
    ///
    /// \param[in] label_to_index_ Translation table from LGR names to LGR
    /// indices.  Empty if there are no LGRs.
    ScheduleGrid(const EclipseGrid&           ecl_grid,
                 const FieldPropsManager&     fpm,
                 CompletedCells&              completed_cells,
                 std::vector<CompletedCells>& completed_cells_lgr,
                 const std::unordered_map<std::string, std::size_t>& label_to_index_);

    /// Make collection aware of numerical aquifers
    ///
    /// Wells intersected in numerical aquifers should have properties from
    /// the numerical aquifer itself rather than from the main property
    /// container.
    ///
    /// \param[in] num_aquifers Run's collection of numerical aquifers.
    void include_numerical_aquifers(const NumericalAquifers& num_aquifers);

    /// Retrieve particular intersected cell in main grid.
    ///
    /// May as a side effect insert a new element into the collection of
    /// CompletedCells, so this operation isn't really 'const' in the strict
    /// sense of the word.  Will throw an exception if unable to create a
    /// new cell object when needed.
    ///
    /// \param[in] i Cell's Cartesian I index relative to main grid's origin.
    /// \param[in] j Cell's Cartesian J index relative to main grid's origin.
    /// \param[in] k Cell's Cartesian K index relative to main grid's origin.
    ///
    /// \return Intersected cell object with associate property data.
    const CompletedCells::Cell&
    get_cell(std::size_t i, std::size_t j, std::size_t k) const;

    /// Retrieve particular intersected cell.
    ///
    /// May as a side effect insert a new element into the collection of
    /// CompletedCells, so this operation isn't really 'const' in the strict
    /// sense of the word.  Will throw an exception if unable to create a
    /// new cell object when needed.
    ///
    /// \param[in] i Cell's Cartesian I index relative to grid's origin.
    ///
    /// \param[in] j Cell's Cartesian J index relative to grid's origin.
    ///
    /// \param[in] k Cell's Cartesian K index relative to grid's origin.
    ///
    /// \param[in] tag Grid identifier.  Nullopt for main grid, and an LGR
    /// name otherwise.
    ///
    /// \return Intersected cell object with associate property data.
    const CompletedCells::Cell&
    get_cell(std::size_t i, std::size_t j, std::size_t k, const std::optional<std::string>& tag) const;

    /// Retrieve underlying grid object.
    ///
    /// Usable only if the ScheduleGrid was created with a reference to a
    /// grid object.
    const EclipseGrid* get_grid() const;

    /// Translate LGR name into a numeric grid index.
    ///
    /// Will throw an exception if the name identifies an unknown LGR.
    ///
    /// \param[in] lgr_label LGR name.  Nullopt for main grid.
    ///
    /// \return Numeric grid index.
    int get_lgr_grid_number(const std::optional<std::string>& lgr_label) const;

private:
    /// Underlying grid object.
    const EclipseGrid* grid{nullptr};

    /// Property container.
    const FieldPropsManager* fp{nullptr};

    /// Collection of intersected cells in main grid.
    ///
    /// Reference to a mutable object that must outlive the ScheduleGrid.
    std::reference_wrapper<CompletedCells> cells;

    /// Collection of intersected cells in LGRs.
    ///
    /// Reference to a mutable object that must outlive the ScheduleGrid.
    std::reference_wrapper<std::vector<CompletedCells>> cells_lgr;

    /// Translation table for LGR names.
    ///
    /// Reference to an immutable object that must outlive the ScheduleGrid.
    std::reference_wrapper<const std::unordered_map<std::string, std::size_t>> label_to_index;

    /// Run's cells, including property data, in numerical aquifers.
    ///
    /// Keyed by Cartesian cell index.
    std::unordered_map<std::size_t, const NumericalAquiferCell*> num_aqu_cells{};

    /// Retrieve particular intersected cell in local grid.
    ///
    /// May as a side effect insert a new element into the collection of
    /// CompletedCells, so this operation isn't really 'const' in the strict
    /// sense of the word.  Will throw an exception if unable to create a
    /// new cell object when needed.
    ///
    /// \param[in] i Cell's Cartesian I index relative to grid's origin.
    ///
    /// \param[in] j Cell's Cartesian J index relative to grid's origin.
    ///
    /// \param[in] k Cell's Cartesian K index relative to grid's origin.
    ///
    /// \param[in] tag Local grid name.
    ///
    /// \return Intersected cell object with associate property data.
    const CompletedCells::Cell&
    get_cell_lgr(std::size_t i, std::size_t j, std::size_t k, const std::string& tag) const;

    /// Populate intersected cell property data for cell in main grid
    ///
    /// Knows how to distinguish regular cells from those that happen to be
    /// in numerical aquifers.
    ///
    /// \param[in,out] cell Intersected cell object.  This function will
    /// populate its Cell::Props sub-object.
    void populate_props_from_main_grid(CompletedCells::Cell& cell) const;

    /// Populate intersected cell property data for cell in main grid.
    ///
    /// Typical case.  The cell is a "regular" cell so we extract property
    /// values from the main property container.
    ///
    /// \param[in,out] cell Intersected cell object.  This function will
    /// populate its Cell::Props sub-object.
    void populate_props_from_main_grid_cell(CompletedCells::Cell& cell) const;

    /// Populate intersected cell property data for cell in main grid.
    ///
    /// Uncommon case.  The cell is in a numerical aquifer so we extract
    /// property values from that aquifer object.
    ///
    /// \param[in] numAquCell Property data pertaining to the particular
    /// numerical aquifer cell.
    ///
    /// \param[in,out] cell Intersected cell object.  This function will
    /// populate its Cell::Props sub-object.
    void populate_props_from_num_aquifer(const NumericalAquiferCell& numAquCell,
                                         CompletedCells::Cell&       cell) const;

    /// Populate intersected cell property data for cell in local grid.
    ///
    /// \param[in] tag Local grid name.
    ///
    /// \param[in,out] cell Intersected cell object in local grid.  This
    /// function will populate its Cell::Props sub-object.
    void populate_props_lgr(const std::string&    tag,
                            CompletedCells::Cell& cell) const;

    /// Retrieve numerical aquifer property data for particular main grid
    /// cell.
    ///
    /// \param[in] global_index Linearised Cartesian cell index in main
    /// grid.
    ///
    /// \return Numerical aquifer property data.  Nullptr if \p global_index
    /// is not in a numerical aquifer.
    const NumericalAquiferCell*
    get_num_aqu_cell(const std::size_t global_index) const;
};

} // namespace Opm

#endif  // SCHEDULE_GRID
