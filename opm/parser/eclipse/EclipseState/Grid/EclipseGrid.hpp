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
        EclipseGrid(const std::string& filename);
        EclipseGrid(const ecl_grid_type * src_ptr);
        EclipseGrid(std::shared_ptr<const RUNSPECSection> runspecSection, std::shared_ptr<const GRIDSection> gridSection);
        EclipseGrid(int nx, int ny , int nz , std::shared_ptr<const GRIDSection> gridSection);

        
        static bool hasCornerPointKeywords(std::shared_ptr<const GRIDSection> gridSection);
        static bool hasCartesianKeywords(std::shared_ptr<const GRIDSection> gridSection);
        void initCartesianGrid(const std::vector<int>& dims , std::shared_ptr<const GRIDSection> gridSection);
        void initCornerPointGrid(const std::vector<int>& dims , std::shared_ptr<const GRIDSection> gridSection);
        size_t  getNumActive( ) const;
        size_t  getNX( ) const;
        size_t  getNY( ) const;
        size_t  getNZ( ) const;
        size_t  getCartesianSize( ) const;
        bool isPinchActive( ) const;
        double getPinchThresholdThickness( ) const;

        void assertGlobalIndex(size_t globalIndex) const;
        void assertIJK(size_t i , size_t j , size_t k) const;
        std::tuple<double,double,double> getCellCenter(size_t i,size_t j, size_t k) const;
        std::tuple<double,double,double> getCellCenter(size_t globalIndex) const;
        double getCellVolume(size_t globalIndex) const;
        double getCellVolume(size_t i , size_t j , size_t k) const;

        void exportMAPAXES( std::vector<double>& mapaxes) const;
        void exportCOORD( std::vector<double>& coord) const;
        void exportZCORN( std::vector<double>& zcorn) const;
        void exportACTNUM( std::vector<int>& actnum) const;
        void resetACTNUM( const int * actnum);
        bool equal(const EclipseGrid& other) const;
        void fwriteEGRID( const std::string& filename ) const;
        const ecl_grid_type * c_ptr() const;
    private:
        std::shared_ptr<ecl_grid_type> m_grid;
        bool m_pinchActive;
        double m_pinchThresholdThickness;

        void assertCornerPointKeywords( const std::vector<int>& dims , std::shared_ptr<const GRIDSection> gridSection ) const ;
        void initDTOPSGrid(const std::vector<int>& dims , std::shared_ptr<const GRIDSection> gridSection);
        void initDVDEPTHZGrid(const std::vector<int>& dims , std::shared_ptr<const GRIDSection> gridSection);
        void initGrid( const std::vector<int>& dims , std::shared_ptr<const GRIDSection> gridSection );
        static bool hasDVDEPTHZKeywords(std::shared_ptr<const GRIDSection> gridSection);
        static bool hasDTOPSKeywords(std::shared_ptr<const         GRIDSection> gridSection);
        static void assertVectorSize(const std::vector<double>& vector , size_t expectedSize , const std::string& msg);
        std::vector<double> createTOPSVector(const std::vector<int>& dims , const std::vector<double>& DZ , std::shared_ptr<const GRIDSection> gridSection);
        std::vector<double> createDVector(const std::vector<int>& dims , size_t dim , const std::string& DKey , const std::string& DVKey, std::shared_ptr<const GRIDSection> gridSection);
        void scatterDim(const std::vector<int>& dims , size_t dim , const std::vector<double>& DV , std::vector<double>& D);
   };

    typedef std::shared_ptr<EclipseGrid> EclipseGridPtr;
    typedef std::shared_ptr<const EclipseGrid> EclipseGridConstPtr;
}




#endif
