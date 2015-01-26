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

#include <opm/parser/eclipse/Log/Logger.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>

#include <ert/ecl/ecl_grid.h>

#include <memory>

namespace Opm {

    /**
       About cell information and dimension: The actual grid
       information is held in a pointer to an ERT ecl_grid_type
       instance. This pointer must be used for access to all cell
       related properties, including:

         - Size of cells
         - Real world position of cells
         - Active/inactive status of cells

       However in may cases the only required information is the
       dimension of the grid. To facilitate simpler use, in particular
       in testing, the grid dimensions are internalized separate from
       the ecl_grid_type pointer. This means that in many cases a grid
       without the underlying ecl_grid_type pointer is sufficient. To
       create such a 'naked' grid you can parse a deck with only
       DIMENS / SPECGRID and no further grid related keywords, or
       alternatively use the:

           EclipseGrid::EclipseGrid(nx,ny,nz)

       constructor.

       To query a grid instance if it has proper underlying grid
       support use the method:

           bool EclipseGrid::hasCellInfo();

    */

    class EclipseGrid {
    public:
        explicit EclipseGrid(const std::string& filename);
        explicit EclipseGrid(const ecl_grid_type * src_ptr);
        explicit EclipseGrid(size_t nx, size_t ny , size_t nz);
        explicit EclipseGrid(std::shared_ptr<const Deck> deck, LoggerPtr logger = std::make_shared<Logger>());

        static bool hasCornerPointKeywords(std::shared_ptr<const Deck> deck);
        static bool hasCartesianKeywords(std::shared_ptr<const Deck> deck);
        size_t  getNumActive( ) const;
        size_t  getNX( ) const;
        size_t  getNY( ) const;
        size_t  getNZ( ) const;
        size_t  getCartesianSize( ) const;
        bool isPinchActive( ) const;
        double getPinchThresholdThickness( ) const;
        bool isMinpvActive( ) const;
        double getMinpvValue( ) const;
        bool hasCellInfo() const;

        void assertGlobalIndex(size_t globalIndex) const;
        void assertIJK(size_t i , size_t j , size_t k) const;
        std::tuple<double,double,double> getCellCenter(size_t i,size_t j, size_t k) const;
        std::tuple<double,double,double> getCellCenter(size_t globalIndex) const;
        double getCellVolume(size_t globalIndex) const;
        double getCellVolume(size_t i , size_t j , size_t k) const;
        bool cellActive( size_t globalIndex ) const;
        bool cellActive( size_t i , size_t , size_t k ) const;
        double getCellDepth(size_t i,size_t j, size_t k) const;
        double getCellDepth(size_t globalIndex) const;


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
        Value<double> m_minpv;
        Value<double> m_pinch;
        size_t m_nx;
        size_t m_ny;
        size_t m_nz;

        void assertCellInfo() const;

        void initCartesianGrid(const std::vector<int>& dims , DeckConstPtr deck);
        void initCornerPointGrid(const std::vector<int>& dims , DeckConstPtr deck, LoggerPtr logger);
        void initDTOPSGrid(const std::vector<int>& dims , DeckConstPtr deck);
        void initDVDEPTHZGrid(const std::vector<int>& dims , DeckConstPtr deck);
        void initGrid(const std::vector<int>& dims, DeckConstPtr deck, LoggerPtr logger);

        static void assertCornerPointKeywords(const std::vector<int>& dims, DeckConstPtr deck, LoggerPtr logger) ;
        static bool hasDVDEPTHZKeywords(DeckConstPtr deck);
        static bool hasDTOPSKeywords(DeckConstPtr deck);
        static void assertVectorSize(const std::vector<double>& vector , size_t expectedSize , const std::string& msg);
        static std::vector<double> createTOPSVector(const std::vector<int>& dims , const std::vector<double>& DZ , DeckConstPtr deck);
        static std::vector<double> createDVector(const std::vector<int>& dims , size_t dim , const std::string& DKey , const std::string& DVKey, DeckConstPtr deck);
        static void scatterDim(const std::vector<int>& dims , size_t dim , const std::vector<double>& DV , std::vector<double>& D);
   };

    typedef std::shared_ptr<EclipseGrid> EclipseGridPtr;
    typedef std::shared_ptr<const EclipseGrid> EclipseGridConstPtr;
}




#endif
