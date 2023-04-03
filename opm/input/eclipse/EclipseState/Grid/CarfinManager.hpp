/*
  Copyright 2022 Equinor
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

#ifndef CARFINMANAGER_HPP_
#define CARFINMANAGER_HPP_

#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <memory>
#include <vector>

namespace Opm {

    class CarfinManager
    {
    public:
        explicit CarfinManager(const GridDims& gridDims,
                            Carfin::IsActive   isActive,
                            Carfin::ActiveIdx  activeIdx);

        void setInputCarfin(std::string name, int i1, int i2, int j1, int j2, int k1, int k2, int nx , int ny , int nz);
        void readKeywordCarfin(std::string name, int i1, int i2, int j1, int j2, int k1, int k2, int nx , int ny , int nz);

        void endSection();
        void endInputCarfin();
        void endKeyword();

        const Carfin& getActiveCarfin() const;
        const std::vector<Carfin::cell_index>& index_list() const;

    private:
        GridDims gridDims_{};
        Carfin::IsActive isActive_{};
        Carfin::ActiveIdx activeIdx_{};

        std::unique_ptr<Carfin> m_globalCarfin;
        std::unique_ptr<Carfin> m_inputCarfin;
        std::unique_ptr<Carfin> m_keywordCarfin;

        std::unique_ptr<Carfin>
        makeLgr(std::string name, int i1, int i2, 
                int j1, int j2, 
                int k1, int k2, 
                int nx , int ny , int nz) const;
    };
}


#endif  
