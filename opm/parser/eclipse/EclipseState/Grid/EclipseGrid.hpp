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


#ifndef OPM_PARSER_ECLIPSE_GRID_HPP
#define OPM_PARSER_ECLIPSE_GRID_HPP


#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/MinpvMode.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/PinchMode.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/parser/eclipse/Parser/MessageContainer.hpp>

#include <ert/ecl/ecl_grid.h>
#include <ert/util/ert_unique_ptr.hpp>

#include <array>
#include <memory>
#include <vector>

namespace Opm {

    class Deck;

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

    class EclipseGrid : public GridDims {
    public:
        explicit EclipseGrid(const std::string& filename);
        explicit EclipseGrid(const EclipseGrid& srcGrid);
	EclipseGrid(size_t nx, size_t ny, size_t nz,
		    double dx = 1.0, double dy = 1.0, double dz = 1.0);
	
	EclipseGrid(const std::vector<int>& dims , 
		    const std::vector<double>& coord , 
		    const std::vector<double>& zcorn , 
		    const int * actnum = nullptr,
		    const double * mapaxes = nullptr);


        /// EclipseGrid ignores ACTNUM in Deck, and therefore needs ACTNUM
        /// explicitly.  If a null pointer is passed, every cell is active.
	EclipseGrid(const Deck& deck, const int * actnum = nullptr);
        /// [deprecated]
	EclipseGrid(const std::shared_ptr<const Deck>& deck, const int * actnum = nullptr);

        static bool hasCornerPointKeywords(const Deck&);
        static bool hasCartesianKeywords(const Deck&);
        size_t  getNumActive( ) const;

        size_t activeIndex(size_t i, size_t j, size_t k) const;
        size_t activeIndex(size_t globalIndex) const;

        bool isPinchActive( ) const;
        double getPinchThresholdThickness( ) const;
        PinchMode::ModeEnum getPinchOption( ) const;
        PinchMode::ModeEnum getMultzOption( ) const;

        MinpvMode::ModeEnum getMinpvMode() const;
        double getMinpvValue( ) const;

        bool hasCellInfo() const;
        std::array<double, 3> getCellCenter(size_t i,size_t j, size_t k) const;
        std::array<double, 3> getCellCenter(size_t globalIndex) const;
        double getCellVolume(size_t globalIndex) const;
        double getCellVolume(size_t i , size_t j , size_t k) const;
        double getCellThicknes(size_t globalIndex) const;
        double getCellThicknes(size_t i , size_t j , size_t k) const;
        std::array<double, 3> getCellDims(size_t i,size_t j, size_t k) const;
        std::array<double, 3> getCellDims(size_t globalIndex) const;
        bool cellActive( size_t globalIndex ) const;
        bool cellActive( size_t i , size_t j, size_t k ) const;
        double getCellDepth(size_t i,size_t j, size_t k) const;
        double getCellDepth(size_t globalIndex) const;


        void exportMAPAXES( std::vector<double>& mapaxes) const;
        void exportCOORD( std::vector<double>& coord) const;
        void exportZCORN( std::vector<double>& zcorn) const;
        void exportACTNUM( std::vector<int>& actnum) const;
        void resetACTNUM( const int * actnum);
        bool equal(const EclipseGrid& other) const;
        void fwriteEGRID( const std::string& filename, bool output_metric ) const;
        const ecl_grid_type * c_ptr() const;
        const MessageContainer& getMessageContainer() const;
        MessageContainer& getMessageContainer();
    private:
        MessageContainer m_messages;

        ERT::ert_unique_ptr<ecl_grid_type , ecl_grid_free> m_grid;
        double m_minpvValue;
        MinpvMode::ModeEnum m_minpvMode;
        Value<double> m_pinch;
        PinchMode::ModeEnum m_pinchoutMode;
        PinchMode::ModeEnum m_multzMode;

        void assertCellInfo() const;
	
	void initCornerPointGrid(const std::vector<int>& dims , 
				 const std::vector<double>& coord , 
				 const std::vector<double>& zcorn , 
				 const int * actnum,
				 const double * mapaxes);
        void initCartesianGrid(const std::vector<int>& dims, const Deck&);
        void initCornerPointGrid(const std::vector<int>& dims, const Deck&);
        void initDTOPSGrid(const std::vector<int>& dims, const Deck&);
        void initDVDEPTHZGrid(const std::vector<int>& dims, const Deck& deck);
        void initGrid(const std::vector<int>& dims, const Deck&);

        void assertCornerPointKeywords(const std::vector<int>& dims, const Deck&);
        static bool hasDVDEPTHZKeywords(const Deck&);
        static bool hasDTOPSKeywords(const Deck&);
        static void assertVectorSize(const std::vector<double>& vector, size_t expectedSize, const std::string& msg);
        static std::vector<double> createTOPSVector(const std::vector<int>& dims, const std::vector<double>& DZ,
                const Deck&);
        static std::vector<double> createDVector(const std::vector<int>& dims, size_t dim, const std::string& DKey,
                const std::string& DVKey, const Deck&);
        static void scatterDim(const std::vector<int>& dims , size_t dim , const std::vector<double>& DV , std::vector<double>& D);
   };

    typedef std::shared_ptr<EclipseGrid> EclipseGridPtr;
    typedef std::shared_ptr<const EclipseGrid> EclipseGridConstPtr;
}




#endif // OPM_PARSER_ECLIPSE_GRID_HPP
