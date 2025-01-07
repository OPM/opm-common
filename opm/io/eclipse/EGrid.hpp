/*
   Copyright 2019 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#ifndef OPM_IO_EGRID_HPP
#define OPM_IO_EGRID_HPP

#include <opm/io/eclipse/EclFile.hpp>

#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace Opm
{
namespace EclIO
{
    struct ENNCConnection {
        int grid1_Id;
        int grid1_CellIdx;
        int grid2_Id;
        int grid2_CellIdx;
        float transValue;

        ENNCConnection(int id1, int cellId1, int id2, int cellId2, float trans)
        {
            grid1_Id = id1;
            grid2_Id = id2;
            grid1_CellIdx = cellId1;
            grid2_CellIdx = cellId2;
            transValue = trans;
        }
    };


    class EGrid : public EclFile
    {
    public:
        explicit EGrid(const std::string& filename, const std::string& grid_name = "global");

        int global_index(int i, int j, int k) const;
        int active_index(int i, int j, int k) const;
        int active_frac_index(int i, int j, int k) const;

        const std::array<int, 3>& dimension() const
        {
            return nijk;
        }

        std::array<int, 3> ijk_from_active_index(int actInd) const;
        std::array<int, 3> ijk_from_active_frac_index(int actFracInd) const;
        std::array<int, 3> ijk_from_global_index(int globInd) const;

        void
        getCellCorners(int globindex, std::array<double, 8>& X, std::array<double, 8>& Y, std::array<double, 8>& Z);

        void getCellCorners(const std::array<int, 3>& ijk,
                            std::array<double, 8>& X,
                            std::array<double, 8>& Y,
                            std::array<double, 8>& Z);

        void getCellCorners(const std::array<int, 3>& ijk,
                            std::array<double, 8>& X,
                            std::array<double, 8>& Y,
                            std::array<double, 8>& Z,
                            bool convert_to_radial_coords);

        std::vector<std::array<float, 3>> getXYZ_layer(int layer, bool bottom = false);
        std::vector<std::array<float, 3>> getXYZ_layer(int layer, const std::array<int, 4>& box, bool bottom = false);

        // number of active cells that are either frac or matrix or both
        int totalActiveCells() const
        {
            return (int)glob_index.size();
        }

        // number of active matrix cells
        int activeCells() const
        {
            return nactive;
        }

        // number of active fracture cells
        int activeFracCells() const
        {
            return nactive_frac;
        }

        int totalNumberOfCells() const
        {
            return nijk[0] * nijk[1] * nijk[2];
        }

        void load_grid_data();
        void load_nnc_data();
        bool with_mapaxes() const
        {
            return m_mapaxes_loaded;
        }
        void mapaxes_transform(double& x, double& y) const;
        bool is_radial() const
        {
            return m_radial;
        }

        // porosity_mode < 0 means not specified, 0 = single, 1 = dual por, 2 = dual perm
        int porosity_mode() const
        {
            return m_porosity_mode;
        }

        std::string grid_unit() const
        {
            return m_grid_unit;
        }

        const std::vector<int>& hostCellsGlobalIndex() const
        {
            return host_cells;
        }
        std::vector<std::array<int, 3>> hostCellsIJK();

        // zero based: i1,j1,k1, i2,j2,k2, transmisibility
        using NNCentry = std::tuple<int, int, int, int, int, int, float>;
        std::vector<NNCentry> get_nnc_ijk();

        const std::vector<std::string>& list_of_lgrs() const
        {
            return lgr_names;
        }

        const std::vector<std::string>& list_of_lgr_parents() const
        {
            return lgr_parents;
        }

        const std::array<double, 6>& get_mapaxes() const
        {
            return m_mapaxes;
        }
        const std::string& get_mapunits() const
        {
            return m_mapunits;
        }

        std::vector<ENNCConnection> nnc_connections(int this_grid_id);

        void set_active_cells(const std::vector<int>& active_cell_info);

        const std::vector<int>& active_indexes() const
        {
            return act_index;
        };
        const std::vector<int>& active_frac_indexes() const
        {
            return act_frac_index;
        };

        const std::vector<float>& get_coord() const
        {
            return coord_array;
        }
        const std::vector<float>& get_zcorn() const
        {
            return zcorn_array;
        }

    private:
        std::filesystem::path inputFileName, initFileName;
        std::string m_grid_name;
        bool m_radial;

        std::array<double, 6> m_mapaxes;
        std::string m_mapunits;
        bool m_mapaxes_loaded;
        std::array<double, 4> origin;
        std::array<double, 2> unit_x;
        std::array<double, 2> unit_y;

        std::array<int, 3> nijk;
        std::array<int, 3> host_nijk;

        int nactive;
        int nactive_frac;

        mutable bool m_nncs_loaded;

        int m_porosity_mode;
        std::string m_grid_unit;

        std::vector<int> act_index;
        std::vector<int> act_frac_index;
        std::vector<int> glob_index;

        std::vector<float> coord_array;
        std::vector<float> zcorn_array;

        std::vector<int> nnc1_array;
        std::vector<int> nnc2_array;
        std::vector<float> transnnc_array;
        std::vector<int> nncg_array;
        std::vector<int> nncl_array;
        std::vector<float> transgl_array;

        std::vector<int> host_cells;
        std::map<int, int> res;
        std::vector<std::string> lgr_names;
        std::vector<std::string> lgr_parents;
        int numres;

        int zcorn_array_index;
        int coord_array_index;
        int coordsys_array_index;

        int actnum_array_index;
        int nnc1_array_index;
        int nnc2_array_index;
        int nncl_array_index;
        int nncg_array_index;

        std::vector<float> get_zcorn_from_disk(int layer, bool bottom);

        void getCellCorners(const std::array<int, 3>& ijk,
                            const std::vector<float>& zcorn_layer,
                            std::array<double, 4>& X,
                            std::array<double, 4>& Y,
                            std::array<double, 4>& Z);
        void mapaxes_init();
    };

} // namespace EclIO
} // namespace Opm

#endif // OPM_IO_EGRID_HPP
