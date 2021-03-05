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

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

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

        DynamicState() = default;

        DynamicState( const TimeMap& timeMap, T initial ) :
            m_data( timeMap.size(), initial ),
            initial_range( timeMap.size() )
        {}

        DynamicState(const std::vector<T>& data,
                     size_t init_range) :
            m_data(data), initial_range(init_range)
        {}

        void globalReset( T value ) {
            this->m_data.assign( this->m_data.size(), value );
        }

        const T& back() const {
            return m_data.back();
        }

        const T& get(size_t index) const {
            return this->m_data.at( index );
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





    bool operator==(const DynamicState<T>& data) const {
        return m_data == data.m_data &&
               initial_range == data.initial_range;
    }

    // complexType=true if contained type has a serializeOp
    template<class Serializer, bool complexType = true>
    void serializeOp(Serializer& serializer)
    {
        std::vector<T> unique;
        auto indices = split(unique);
        serializer.template vector<T,complexType>(unique);
        serializer(indices);
        if (!serializer.isSerializing())
            reconstruct(unique, indices);
    }

    private:
        std::vector< T > m_data;
        size_t initial_range;

        std::vector<size_t> split(std::vector<T>& unique) const {
            std::vector<size_t> idxVec;
            idxVec.reserve(m_data.size() + 1);
            for (const auto& w : m_data) {
                auto candidate = std::find(unique.begin(), unique.end(), w);
                size_t idx = candidate - unique.begin();
                if (candidate == unique.end()) {
                    unique.push_back(w);
                    idx = unique.size() - 1;
                }
                idxVec.push_back(idx);
            }
            idxVec.push_back(initial_range);

            return idxVec;
        }

        void reconstruct(const std::vector<T>& unique,
                         const std::vector<size_t>& idxVec) {
            m_data.clear();
            m_data.reserve(idxVec.size() - 1);
            for (size_t i = 0; i < idxVec.size() - 1; ++i)
                m_data.push_back(unique[idxVec[i]]);

            initial_range = idxVec.back();
        }
};

}

#endif

