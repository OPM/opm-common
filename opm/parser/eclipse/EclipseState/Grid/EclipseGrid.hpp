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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>

#include <ert/ecl/ecl_grid.h>

#include <memory>

namespace Opm {

    class EclipseGrid {
    public:
        explicit EclipseGrid(const std::string& filename);
        explicit EclipseGrid(const ecl_grid_type * src_ptr);
        explicit EclipseGrid(std::shared_ptr<const Deck> deck);

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
        bool m_minpvActive;
        double m_minpvValue;

        void initCartesianGrid(const std::vector<int>& dims , DeckConstPtr deck);
        void initCornerPointGrid(const std::vector<int>& dims , DeckConstPtr deck);
        void assertCornerPointKeywords( const std::vector<int>& dims , DeckConstPtr deck ) const ;
        void initDTOPSGrid(const std::vector<int>& dims , DeckConstPtr deck);
        void initDVDEPTHZGrid(const std::vector<int>& dims , DeckConstPtr deck);
        void initGrid( const std::vector<int>& dims , DeckConstPtr deck );
        static bool hasDVDEPTHZKeywords(DeckConstPtr deck);
        static bool hasDTOPSKeywords(DeckConstPtr deck);
        static void assertVectorSize(const std::vector<double>& vector , size_t expectedSize , const std::string& msg);
        std::vector<double> createTOPSVector(const std::vector<int>& dims , const std::vector<double>& DZ , DeckConstPtr deck);
        std::vector<double> createDVector(const std::vector<int>& dims , size_t dim , const std::string& DKey , const std::string& DVKey, DeckConstPtr deck);
        void scatterDim(const std::vector<int>& dims , size_t dim , const std::vector<double>& DV , std::vector<double>& D);
   };

    typedef std::shared_ptr<EclipseGrid> EclipseGridPtr;
    typedef std::shared_ptr<const EclipseGrid> EclipseGridConstPtr;
}




#endif
