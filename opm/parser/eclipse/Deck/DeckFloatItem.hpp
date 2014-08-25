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

#ifndef DECKFLOATITEM_HPP
#define DECKFLOATITEM_HPP

#include <vector>
#include <string>
#include <deque>
#include <memory>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>

namespace Opm {

    class DeckFloatItem : public DeckItem {
    public:
        DeckFloatItem(std::string name_, bool scalar = true) : DeckItem(name_, scalar) {}
        float getRawFloat(size_t index) const;
        float getSIFloat(size_t index) const;
        const std::vector<float>& getSIFloatData() const;

        void push_back(std::deque<float> data , size_t items);
        void push_back(std::deque<float> data);
        void push_back(float value);
        void push_backDefault(float value);
        void push_backMultiple(float value, size_t numValues);
        void push_backDimension(std::shared_ptr<const Dimension> activeDimension , std::shared_ptr<const Dimension> defaultDimension);

        size_t size() const;
    private:
        void assertSIData() const;

        std::vector<float> m_data;
        // mutable is required because the data is "lazily" converted
        // to SI units in asserSIData() which needs to be callable by
        // 'const'-decorated methods
        mutable std::vector<float> m_SIdata;
        std::vector<std::shared_ptr<const Dimension> > m_dimensions;
    };
    typedef std::shared_ptr<DeckFloatItem> DeckFloatItemPtr;
    typedef std::shared_ptr< const DeckFloatItem> DeckFloatItemConstPtr;
}

#endif // DECKFLOATITEM_HPP
