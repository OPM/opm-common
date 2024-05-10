/*
  Copyright 2019 Equinor.
  Copyright 2017 Statoil ASA.
  Copyright 2016 Statoil ASA.

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

#ifndef OUTPUT_TABLES_HPP
#define OUTPUT_TABLES_HPP

#include <tuple>
#include <vector>

namespace Opm {

    struct DensityTable;
    class EclipseState;
    class UnitSystem;

} // namespace Opm

namespace Opm {

    /// Aggregator for run's tabulated functions.
    ///
    /// Forms INIT file's TABDIMS and TAB arrays.
    class Tables
    {
    public:
        /// Constructor.
        ///
        /// \param[in] units Run's active unit system.  Needed to convert SI
        ///    values of pressures, densities, viscosities &c to run's
        ///    output units.
        explicit Tables(const UnitSystem& units);

        /// Incorporate phase densities at surface conditions into INIT
        /// file's TAB vector.
        ///
        /// \param[in] density Run's phase densities at surface conditions,
        ///    typically from the DENSITY keyword.
        void addDensity(const DensityTable& density);

        /// Add normalised PVT function tables to INIT file's TAB vector.
        ///
        /// \param[in] es Valid \c EclipseState object with accurate RUNSPEC
        ///    information on active phases and table dimensions ("TABDIMS").
        void addPVTTables(const EclipseState& es);

        /// Add normalised saturation function tables to INIT file's TAB
        /// vector.
        ///
        /// \param[in] es Valid \c EclipseState object with accurate RUNSPEC
        ///    information on active phases and table dimensions ("TABDIMS").
        void addSatFunc(const EclipseState& es);

        /// Acquire read-only reference to internal TABDIMS vector.
        const std::vector<int>& tabdims() const;

        /// Acquire read-only reference to internal TAB vector.
        const std::vector<double>& tab() const;

    private:
        /// Convention for units of measure of the result set.
        const UnitSystem& units_;

        /// Offset and size information for the tabular data.
        std::vector<int> tabdims_;

        /// Linearised tabular data of PVT and saturation functions.
        std::vector<double> data_;

        /// Incorporate a new table into internal data array and attribute
        /// table values to particular item in dimension array.
        ///
        /// \param[in] offset_index Item in \c m_tabdims which describes the
        ///    start point of this linearised table.
        ///
        /// \param[in] new_data Linearised table values.  Appended to
        ///    internal \c data array.
        void addData(const std::size_t          offset_index,
                     const std::vector<double>& new_data);

        /// Add saturation functions for gas (keywords SGFN, SGOF &c) to the
        /// tabular data (TABDIMS and TAB vectors).
        ///
        /// \param[in] sgfn Table description and normalised table values
        ///    for the run's saturation functions for gas.  Element at index
        ///    zero (i.e., \code get<0>(sgfn) \endcode) is the number of
        ///    allocated rows of each individual SGFN table--typically
        ///    NSSFUN=TABDIMS(3).  Element at index 1 (i.e., \code
        ///    get<1>(sgfn) \endcode) is the number of individual tables.
        ///    Normalised table data is at index 2.
        void addSatFuncGas(const std::tuple<int, int, std::vector<double>>& sgfn);

        /// Add saturation functions for oil (keywords SOF2, SOF3, SGOF,
        /// SWOF &c) to the tabular data (TABDIMS and TAB vectors).
        ///
        /// \param[in] sofn Table description and normalised table values
        ///    for the run's saturation functions for oil.  Element at index
        ///    zero (i.e., \code get<0>(sofn) \endcode) is the number of
        ///    allocated rows of each individual SOFN table--typically
        ///    NSSFUN=TABDIMS(3).  Element at index 1 (i.e., \code
        ///    get<1>(sofn) \endcode) is the number of individual tables.
        ///    Normalised table data is at index 2.
        void addSatFuncOil(const std::tuple<int, int, std::vector<double>>& sofn);

        /// Add saturation functions for water (keywords SWFN, SWOF &c) to
        /// the tabular data (TABDIMS and TAB vectors).
        ///
        /// \param[in] swfn Table description and normalised table values
        ///    for the run's saturation functions for water.  Element at
        ///    index zero (i.e., \code get<0>(swfn) \endcode) is the number
        ///    of allocated rows of each individual SWFN table--typically
        ///    NSSFUN=TABDIMS(3).  Element at index 1 (i.e., \code
        ///    get<1>(swfn) \endcode) is the number of individual tables.
        ///    Normalised table data is at index 2.
        void addSatFuncWater(const std::tuple<int, int, std::vector<double>>& swfn);

        /// Add gas PVT tables (keywords PVDG and PVTG) to the tabular data
        /// (TABDIMS and TAB vectors).
        ///
        /// \param[in] es Valid \c EclipseState object with accurate table
        ///    dimensions ("TABDIMS" keyword) and an initialised \c
        ///    TableManager sub-object.
        void addGasPVTTables(const EclipseState& es);

        /// Add oil PVT tables (keywords PVCDO, PVDO and PVTO) to the
        /// tabular data (TABDIMS and TAB vectors).
        ///
        /// \param[in] es Valid \c EclipseState object with accurate table
        ///    dimensions ("TABDIMS" keyword) and an initialised \c
        ///    TableManager sub-object.
        void addOilPVTTables(const EclipseState& es);

        /// Add water PVT tables (keyword PVTW) to the tabular data (TABDIMS
        /// and TAB vectors).
        ///
        /// \param[in] es Valid \c EclipseState object with accurate table
        ///    dimensions ("TABDIMS" keyword) and an initialised \c
        ///    TableManager sub-object.
        void addWaterPVTTables(const EclipseState& es);
    };

} // namespace Opm

#endif // OUTPUT_TABLES_HPP
