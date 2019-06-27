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
#ifndef ECLIPSE_GRIDPROPERTY_HPP_
#define ECLIPSE_GRIDPROPERTY_HPP_

#include <functional>
#include <string>
#include <utility>
#include <vector>

/*
  This class implemenents a class representing properties which are
  define over an ECLIPSE grid, i.e. with one value for each logical
  cartesian cell in the grid.
*/

namespace Opm {

    class Box;
    class DeckItem;
    class DeckKeyword;
    class EclipseGrid;
    class TableManager;
    template< typename > class GridProperties;

template< typename T >
class GridPropertySupportedKeywordInfo {

    public:
        GridPropertySupportedKeywordInfo() = default;

        using init = std::function< std::vector< T >( size_t ) >;
        using post = std::function<
            void(const std::vector<bool>& defaulted,
                 std::vector< T >&)
        >;

        GridPropertySupportedKeywordInfo(
            const std::string& name,
            init initializer,
            post  postProcessor,
            const std::string& dimString,
            bool m_defaultInitializable = false );

        GridPropertySupportedKeywordInfo(
                const std::string& name,
                init initializer,
                const std::string& dimString,
                bool m_defaultInitializable = false );

        /* this is a convenience constructor which can be used if the default
         * value for the grid property is just a constant.
         */
        GridPropertySupportedKeywordInfo(
                const std::string& name,
                const T defaultValue,
                const std::string& dimString,
                bool m_defaultInitializable = false );

        GridPropertySupportedKeywordInfo(
                const std::string& name,
                const T defaultValue,
                post postProcessor,
                const std::string& dimString,
                bool m_defaultInitializable = false );

        const std::string& getKeywordName() const;
        const std::string& getDimensionString() const;
        const init& initializer() const;
        const post& postProcessor() const;
        bool isDefaultInitializable() const;

        void setPostProcessor(post processor)
        {
            this->m_postProcessor = std::move(processor);
        }

    private:

        std::string m_keywordName;
        init m_initializer;
        post m_postProcessor;
        std::string m_dimensionString;
        bool m_defaultInitializable;
};

template< typename T >
class GridProperty {
public:
    typedef GridPropertySupportedKeywordInfo<T> SupportedKeywordInfo;

    GridProperty( size_t nx, size_t ny, size_t nz, const SupportedKeywordInfo& kwInfo );

    size_t getCartesianSize() const;
    size_t getNX() const;
    size_t getNY() const;
    size_t getNZ() const;


    T iget(size_t index) const;
    T iget(size_t i , size_t j , size_t k) const;
    void iset(size_t index, T value);
    void iset(size_t i , size_t j , size_t k , T value);


    const std::vector<bool>& wasDefaulted() const;
    const std::vector<T>& getData() const;
    std::vector<T>& getData();

    bool containsNaN() const;
    const std::string& getDimensionString() const;

    void multiplyWith( const GridProperty<T>& );
    void multiplyValueAtIndex( size_t index, T factor );
    void maskedSet( T value, const std::vector< bool >& mask );
    void maskedMultiply( T value, const std::vector< bool >& mask );
    void maskedAdd( T value, const std::vector< bool >& mask );
    void maskedCopy( const GridProperty< T >& other, const std::vector< bool >& mask );
    void initMask( T value, std::vector<bool>& mask ) const;

    /**
       Due to the convention where it is only necessary to supply the
       top layer of the petrophysical properties we can unfortunately
       not enforce that the number of elements elements in the
       DeckKeyword equals nx*ny*nz.
    */

    void loadFromDeckKeyword( const DeckKeyword& );
    void loadFromDeckKeyword( const Box&, const DeckKeyword& );

    void copyFrom( const GridProperty< T >&, const Box& );
    void scale( T scaleFactor, const Box& );
    void maxvalue( T value, const Box& );
    void minvalue( T value, const Box& );
    void add( T shiftValue, const Box& );
    void setScalar( T value, const Box& );

    const std::string& getKeywordName() const;
    const SupportedKeywordInfo& getKeywordInfo() const;

    /**
       Will check that all elements in the property are in the closed
       interval [min,max].
    */
    void checkLimits( T min, T max ) const;

    /*
      The runPostProcessor() method is public; and it is no harm in
      calling it from arbitrary locations. But the intention is that
      should only be called from the Eclipse3DProperties class
      assembling the properties.
    */
    void runPostProcessor();
     /*
      Will scan through the roperty and return a vector of all the
      indices where the property value agrees with the input value.
     */
     std::vector<size_t> indexEqual(T value) const;


     /*
       Will run through all the cells in the activeMap and return a
       list of the elements where the property value agrees with the
       input value. The values returned will be in the space
       [0,nactive) - i.e. 'active' indices.
     */
     std::vector<size_t> cellsEqual(T value , const std::vector<int>& activeMap) const;

     /*
       If active == true the method will get the activeMap from the
       grid and call the cellsEqual( T , std::vector<int>&) overload,
       otherwise it will return indexEqual( value );
     */
     std::vector<size_t>  cellsEqual(T value, const EclipseGrid& grid, bool active = true)  const; 
    /*
      Will return a std::vector<T> of the data in the active cells.
    */
     std::vector<T> compressedCopy( const EclipseGrid& grid) const;


    /*
      The grid properties like PORO and SATNUM can be created in essentially two
      ways; either they can be explicitly assigned in the deck as:

        PORO
           1000*0.15 /

      or they can be created indirectly through various mathematical operations
      like:

         MULTIPLY
            TRANX 0.20 /
         /

      The deckAssigned() property is used so that we can differentiate between
      properties which have been explicitly assigned/loaded from the deck, and
      those where the default construction has been invoked. This functionality
      is implemented purely to support the TRAN? keywords. The transmissibility
      is be default calculated by the simulator code, but the TRAN keywords can
      be used in the deck to manipulate this calculation in two different ways:

         1. TRAN explicitly assigned in GRID section
         ===========================================
         ...
         TRANX
            1000*0.026 /

         COPY
            'TRANX'  'TRANY' /
         /

         In this case the simulator should detect that the input deck has TRANX
         specified and just use the input values from the deck. This is the
         normal handling of keywords, and agrees with e.g. PERMX and PORO. This
         also applies when the COPY keyword has been used, as in the case of
         'TRANY' above.


         2. TRAN modifier present in EDIT section
         ========================================
         The scenario here is that there is no mention of TRANX in the GRID
         section, however the EDIT section contains modifiers like this:

         MULTIPLY
             TRANX  0.25 /

         I.e. we request the TRANX values to be reduced with a factor of 0.25. In
         this case the simulator should still calculate transmissibility according
         to it's normal algorithm, and then subsequently scale that result with a
         factor of 0.25.

         In this case the input layer needs to autocreate a TRANX keyword,
         defaulted to 1.0 and then scale that to 0.25.


      Now - the important point is that when doing transmissibility calculations
      the simulator must be able to distinguish between cases 1 and 2,
      specifically whether the TRANX keyword should be interpreted as absolute
      values(case 1) or as a multiplier(case 2). That is the purpose of the
      deckAssigned() property. Pseudo code for the transmissibility calculations
      in the simulator could be:


          const auto& input_tranx = properties.getKeyword("TRANX");
          if (input_tranx.deckAssigned()) {
               // set simulator internal transmissibilities to values from input_tranx
               tranx = input_tranx;
          } else {
               // Calculate transmissibilities according to normal simulator algorithm
               ...
               ...
               // Scale transmissibilities with scale factor from input_tranx
               tranx *= input_tranx;
          }

     */
    bool deckAssigned() const;

private:
    const DeckItem& getDeckItem( const DeckKeyword& );
    void setDataPoint(size_t sourceIdx, size_t targetIdx, const DeckItem& deckItem);

    size_t m_nx, m_ny, m_nz;
    SupportedKeywordInfo m_kwInfo;
    std::vector<T> m_data;
    std::vector<bool> m_defaulted;
    bool m_hasRunPostProcessor = false;
    bool assigned = false;
};

// initialize the TEMPI grid property using the temperature vs depth
// table (stemming from the TEMPVD or the RTEMPVD keyword)
std::vector< double > temperature_lookup( size_t,
                                          const TableManager*,
                                          const EclipseGrid*,
                                          const GridProperties<int>* );
}

#endif
