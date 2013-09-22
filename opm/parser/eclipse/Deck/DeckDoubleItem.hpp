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

#ifndef DECKDOUBLEITEM_HPP
#define DECKDOUBLEITEM_HPP

#include <vector>
#include <string>
#include <deque>
#include <boost/shared_ptr.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>

namespace Opm {

    class DeckDoubleItem : public DeckItem {
    public:
        DeckDoubleItem(std::string name) : DeckItem(name) {}
        double getDouble(size_t index) const;
        const std::vector<double>& getDoubleData() const;
        
        void push_back(std::deque<double> data , size_t items);
        void push_back(std::deque<double> data);
        void push_back(double value);
        
        size_t size() const;
    private:
        std::vector<double> m_data;
    };

    typedef boost::shared_ptr<DeckDoubleItem> DeckDoubleItemPtr;
    typedef boost::shared_ptr<const DeckDoubleItem> DeckDoubleItemConstPtr;
}
#endif  /* DECKDOUBLEITEM_HPP */

