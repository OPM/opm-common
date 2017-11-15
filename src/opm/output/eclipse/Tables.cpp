/*
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

#include <ert/ecl/FortIO.hpp>
#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/ecl_kw_magic.h>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/output/eclipse/Tables.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace Opm {

    Tables::Tables(const UnitSystem& units)
        : units_  (units)
        , tabdims_(TABDIMS_SIZE, 0)
    {
        // Initialize subset of base pointers and dimensions to 1 to honour
        // requirements of TABDIMS protocol.  The magic constant 59 is
        // derived from the file-formats documentation.
        std::fill_n(std::begin(this->tabdims_), 59, 1);
    }

    void Tables::addData(const std::size_t          offset_index,
                         const std::vector<double>& new_data)
    {
        this->tabdims_[ offset_index ] = this->data_.size() + 1;

        this->data_.insert(this->data_.end(), new_data.begin(), new_data.end());

        this->tabdims_[ TABDIMS_TAB_SIZE_ITEM ] = this->data_.size();
    }


    namespace {
        struct PvtxDims {
            size_t num_tables;
            size_t outer_size;
            size_t inner_size;
            size_t num_columns = 3;
            size_t data_size;
        };

        template <class TableType>
        PvtxDims tableDims( const std::vector<TableType>& pvtxTables) {
            PvtxDims dims;

            dims.num_tables = pvtxTables.size();
            dims.inner_size = 0;
            dims.outer_size = 0;
            for (const auto& table : pvtxTables) {
                dims.outer_size = std::max( dims.outer_size, table.size());
                for (const auto& underSatTable : table)
                    dims.inner_size = std::max(dims.inner_size, underSatTable.numRows() );
            }
            dims.data_size = dims.num_tables * dims.outer_size * dims.inner_size * dims.num_columns;

            return dims;
        }
    }

    void Tables::addPVTO( const std::vector<PvtoTable>& pvtoTables)
    {
        const double default_value = 2e20;
        PvtxDims dims = tableDims( pvtoTables );
        this->tabdims_[ TABDIMS_NTPVTO_ITEM ] = dims.num_tables;
        this->tabdims_[ TABDIMS_NRPVTO_ITEM ] = dims.outer_size;
        this->tabdims_[ TABDIMS_NPPVTO_ITEM ] = dims.inner_size;

        {
            std::vector<double> pvtoData( dims.data_size , default_value );
            std::vector<double> rs_values( dims.num_tables * dims.outer_size  , default_value );
            size_t composition_stride = dims.inner_size;
            size_t table_stride = dims.outer_size * composition_stride;
            size_t column_stride = table_stride * pvtoTables.size();

            size_t table_index = 0;
            for (const auto& table : pvtoTables) {
                size_t composition_index = 0;
                for (const auto& underSatTable : table) {
                    const auto& p  = underSatTable.getColumn("P");
                    const auto& bo = underSatTable.getColumn("BO");
                    const auto& mu = underSatTable.getColumn("MU");

                    for (size_t row = 0; row < p.size(); row++) {
                        size_t data_index = row + composition_stride * composition_index + table_stride * table_index;

                        pvtoData[ data_index ]                  = this->units_.from_si( UnitSystem::measure::pressure, p[row]);
                        pvtoData[ data_index + column_stride ]  = 1.0 / bo[row];
                        pvtoData[ data_index + 2*column_stride] = this->units_.from_si( UnitSystem::measure::viscosity , mu[row]) / bo[row];
                    }
                    composition_index++;
                }

                /*
                  The RS values which apply for one inner table each
                  are added as a separate data vector to the TABS
                  array.
                */
                {
                    const auto& sat_table = table.getSaturatedTable();
                    const auto& rs = sat_table.getColumn("RS");
                    for (size_t index = 0; index < rs.size(); index++)
                        rs_values[index + table_index * dims.outer_size ] = rs[index];
                }
                table_index++;
            }

            this->addData( TABDIMS_IBPVTO_OFFSET_ITEM , pvtoData );
            this->addData( TABDIMS_JBPVTO_OFFSET_ITEM , rs_values );
        }
    }

    void Tables::addPVTG( const std::vector<PvtgTable>& pvtgTables) {
        const double default_value = -2e20;
        PvtxDims dims = tableDims( pvtgTables );
        this->tabdims_[ TABDIMS_NTPVTG_ITEM ] = dims.num_tables;
        this->tabdims_[ TABDIMS_NRPVTG_ITEM ] = dims.outer_size;
        this->tabdims_[ TABDIMS_NPPVTG_ITEM ] = dims.inner_size;

        {
            std::vector<double> pvtgData( dims.data_size , default_value );
            std::vector<double> p_values( dims.num_tables * dims.outer_size  , default_value );
            size_t composition_stride = dims.inner_size;
            size_t table_stride = dims.outer_size * composition_stride;
            size_t column_stride = table_stride * dims.num_tables;

            size_t table_index = 0;
            for (const auto& table : pvtgTables) {
                size_t composition_index = 0;
                for (const auto& underSatTable : table) {
                    const auto& col0 = underSatTable.getColumn(0);
                    const auto& col1 = underSatTable.getColumn(1);
                    const auto& col2 = underSatTable.getColumn(2);

                    for (size_t row = 0; row < col0.size(); row++) {
                        size_t data_index = row + composition_stride * composition_index + table_stride * table_index;

                        pvtgData[ data_index ]                  = this->units_.from_si( UnitSystem::measure::gas_oil_ratio, col0[row]);
                        pvtgData[ data_index + column_stride ]  = this->units_.from_si( UnitSystem::measure::gas_oil_ratio, col1[row]);
                        pvtgData[ data_index + 2*column_stride] = this->units_.from_si( UnitSystem::measure::viscosity , col2[row]);
                    }

                    composition_index++;
                }

                {
                    const auto& sat_table = table.getSaturatedTable();
                    const auto& p = sat_table.getColumn("PG");
                    for (size_t index = 0; index < p.size(); index++)
                        p_values[index + table_index * dims.outer_size ] =
                            this->units_.from_si( UnitSystem::measure::pressure , p[index]);
                }

                table_index++;
            }

            this->addData( TABDIMS_IBPVTG_OFFSET_ITEM , pvtgData );
            this->addData( TABDIMS_JBPVTG_OFFSET_ITEM , p_values );
        }
    }

    void Tables::addPVTW( const PvtwTable& pvtwTable)
    {
        if (pvtwTable.size() > 0) {
            const double default_value = -2e20;
            const size_t num_columns = pvtwTable[0].size;
            std::vector<double> pvtwData( pvtwTable.size() * num_columns , default_value);

            this->tabdims_[ TABDIMS_NTPVTW_ITEM ] = pvtwTable.size();
            for (size_t table_num = 0; table_num < pvtwTable.size(); table_num++) {
                const auto& record = pvtwTable[table_num];
                pvtwData[ table_num * num_columns ]    = this->units_.from_si( UnitSystem::measure::pressure , record.reference_pressure);
                pvtwData[ table_num * num_columns + 1] = 1.0 / record.volume_factor;
                pvtwData[ table_num * num_columns + 2] = this->units_.to_si( UnitSystem::measure::pressure, record.compressibility);
                pvtwData[ table_num * num_columns + 3] = record.volume_factor / this->units_.from_si( UnitSystem::measure::viscosity , record.viscosity);


                // The last column should contain information about
                // the viscosibility, however there is clearly a
                // not-yet-identified transformation involved, we
                // therefor leave this item defaulted.

                // pvtwData[ table_num * num_columns + 4] = record.viscosibility;
            }
            this->addData( TABDIMS_IBPVTW_OFFSET_ITEM , pvtwData );
        }
    }

    void Tables::addDensity( const DensityTable& density)
    {
        if (density.size() > 0) {
            const double default_value = -2e20;
            const size_t num_columns = density[0].size;
            std::vector<double> densityData( density.size() * num_columns , default_value);

            this->tabdims_[ TABDIMS_NTDENS_ITEM ] = density.size();
            for (size_t table_num = 0; table_num < density.size(); table_num++) {
                const auto& record = density[table_num];
                densityData[ table_num * num_columns ]    = this->units_.from_si( UnitSystem::measure::density , record.oil);
                densityData[ table_num * num_columns + 1] = this->units_.from_si( UnitSystem::measure::density , record.water);
                densityData[ table_num * num_columns + 2] = this->units_.from_si( UnitSystem::measure::density , record.gas);
            }
            this->addData( TABDIMS_IBDENS_OFFSET_ITEM , densityData );
        }
    }


    const std::vector<int>& Tables::tabdims() const
    {
        return this->tabdims_;
    }

    const std::vector<double>& Tables::tab() const
    {
        return this->data_;
    }

    void fwrite(const Tables& tables,
                ERT::FortIO&  fortio)
    {
        {
            ERT::EclKW<int> tabdims("TABDIMS", tables.tabdims());
            tabdims.fwrite(fortio);
        }

        {
            ERT::EclKW<double> tab("TAB", tables.tab());
            tab.fwrite(fortio);
        }
    }
}
