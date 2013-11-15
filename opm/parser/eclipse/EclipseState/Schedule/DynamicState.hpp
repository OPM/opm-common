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

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>
#include <stdexcept>


namespace Opm {

    template <class T>
    class DynamicState {
    public:

        DynamicState(const TimeMapConstPtr timeMap, T defaultValue) {
            m_timeMap = timeMap;
            m_currentValue = defaultValue;
        }


        T get(size_t index) const {
            if (index >= m_timeMap->size())
                throw std::range_error("Index value is out range.");
            
            if (index >= m_data.size())
                return m_currentValue;
            
            return m_data[index];
        }


        size_t size() const {
            return m_data.size();
        }


        void add(size_t index , T value) {
            if (index >= (m_timeMap->size()))
                throw std::range_error("Index value is out range.");
 
           if (m_data.size() > 0) {
                if (index < (m_data.size() - 1))
                    throw std::invalid_argument("Elements must be added in weakly increasing order");
            }
           
           {
               size_t currentSize = m_data.size();
               if (currentSize <= index) {
                   for (size_t i = currentSize; i <= index; i++)
                       m_data.push_back( m_currentValue );
               }
           }
           
           m_data[index] = value;
           m_currentValue = value;
        }



    private:
        std::vector<T> m_data;
        TimeMapConstPtr m_timeMap;
        T m_currentValue;
    };
}



#endif 
