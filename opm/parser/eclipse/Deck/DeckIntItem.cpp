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

#include <boost/lexical_cast.hpp>

#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>

namespace Opm {

    int DeckIntItem::getInt(size_t index) const {
        assertSize(index);

        return m_data[index];
    }


    const std::vector<int>& DeckIntItem::getIntData() const {
        return m_data;
    }

    void DeckIntItem::push_back(std::deque<int> data, size_t items) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        for (size_t i = 0; i < items; i++) {
            m_data.push_back(data[i]);
            m_dataPointDefaulted.push_back(false);
        }
    }

    void DeckIntItem::push_back(std::deque<int> data) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        push_back(data, data.size());
        m_dataPointDefaulted.push_back(false);
    }

    void DeckIntItem::push_back(int data) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        m_data.push_back(data);
        m_dataPointDefaulted.push_back(false);
    }

    void DeckIntItem::push_backDefault(int data) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        m_data.push_back( data );
        m_dataPointDefaulted.push_back(true);
    }

    void DeckIntItem::push_backDummyDefault() {
        if (m_dataPointDefaulted.size() != 0)
            throw std::logic_error("Pseudo defaults can only be specified for empty items");

        m_dataPointDefaulted.push_back(true);
    }

    void DeckIntItem::push_backMultiple(int value, size_t numValues) {
        if (m_dataPointDefaulted.size() != m_data.size())
            throw std::logic_error("To add a value to an item, no \"pseudo defaults\" can be added before");

        for (size_t i = 0; i < numValues; i++) {
            m_data.push_back( value );
            m_dataPointDefaulted.push_back(false);
        }
    }


    size_t DeckIntItem::size() const {
        return m_data.size();
    }

}

