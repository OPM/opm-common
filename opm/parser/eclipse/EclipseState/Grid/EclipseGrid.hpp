/*
  Copyright 2014 Statoil ASA.

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


#ifndef ECLIPSE_GRID_HPP_
#define ECLIPSE_GRID_HPP_

#include <opm/parser/eclipse/Deck/Section.hpp>

#include <ert/ecl/ecl_grid.h>

#include <memory>

namespace Opm {

    class EclipseGrid {
    public:
        EclipseGrid(std::shared_ptr<const RUNSPECSection> runspecSection, std::shared_ptr<const GRIDSection> gridSection);
        
        static bool hasCornerPointKeywords(std::shared_ptr<const GRIDSection> gridSection);
        static bool hasCartesianKeywords(std::shared_ptr<const GRIDSection> gridSection);
        void initCartesianGrid(const std::vector<int>& dims , std::shared_ptr<const GRIDSection> gridSection);
        int  getNX( ) const;
        int  getNY( ) const;
        int  getNZ( ) const;
                    
    private:
        std::shared_ptr<ecl_grid_type> m_grid;


        std::vector<double> createTOPSVector(const std::vector<int>& dims , const std::vector<double>& DZ , std::shared_ptr<const GRIDSection> gridSection);
        std::vector<double> createDVector(const std::vector<int>& dims , size_t dim , const std::string& DKey , const std::string& DVKey, std::shared_ptr<const GRIDSection> gridSection);
        void scatterDim(const std::vector<int>& dims , size_t dim , const std::vector<double>& DV , std::vector<double>& D);
   };

    typedef std::shared_ptr<EclipseGrid> EclipseGridPtr;
    typedef std::shared_ptr<const EclipseGrid> EclipseGridConstPtr;
}




#endif
