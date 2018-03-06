/*
  Copyright 2013 Statoil ASA.

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


#ifndef DYNAMICSTATE_HPP_
#define DYNAMICSTATE_HPP_

#include <stdexcept>
#include <vector>
#include <algorithm>

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>


namespace Opm {

    /**
       The DynamicState<T> class is designed to hold information about
       properties with the following semantics:

         1. The property can be updated repeatedly at different
            timesteps; observe that the class does not support
            operator[] - only updates with weakly increasing timesteps
            are supported.

         2. At any point in the time the previous last set value
            applies.

       The class is very much tailored to support the Schedule file of
       Eclipse where a control applied at time T will apply
       indefinitely, or until explicitly set to a different value.

       The update() method returns true if the updated value is
       different from the current value, this implies that the
       class<T> must support operator!=
    */



template< class T >
class DynamicState {

    public:
        typedef typename std::vector< T >::iterator iterator;

        DynamicState( const TimeMap& timeMap, T initial ) :
            m_data( timeMap.size(), initial ),
            initial_range( timeMap.size() )
        {}

        void globalReset( T value ) {
            this->m_data.assign( this->m_data.size(), value );
        }

        const T& back() const {
            return m_data.back();
        }

        const T& at( size_t index ) const {
            return this->m_data.at( index );
        }

        const T& operator[](size_t index) const {
            return this->at( index );
        }

        const T& get(size_t index) const {
            return this->at( index );
        }

        void updateInitial( T initial ) {
            std::fill_n( this->m_data.begin(), this->initial_range, initial );
        }

        /**
           If the current value has been changed the method will
           return true, otherwise it will return false.
        */
        bool update( size_t index, T value ) {
            if( this->initial_range == this->m_data.size() )
                this->initial_range = index;

            const bool change = (value != this->m_data.at( index ));

            if( !change ) return false;

            std::fill( this->m_data.begin() + index,
                       this->m_data.end(),
                       value );

            return true;
        }

        void update_elm( size_t index, const T& value ) {
            if (this->m_data.size() <= index)
                throw std::out_of_range("Invalid index for update_elm()");

            this->m_data[index] = value;
        }

        /// Will return the index of the first occurence of @value, or
        /// -1 if @value is not found.
        int find(const T& value) const {
            auto iter = std::find( m_data.begin() , m_data.end() , value);
            if( iter == this->m_data.end() ) return -1;

            return std::distance( m_data.begin() , iter );
        }



        iterator begin() {
            return this->m_data.begin();
        }


        iterator end() {
            return this->m_data.end();
        }

    private:
        std::vector< T > m_data;
        size_t initial_range;
};

}

#endif

