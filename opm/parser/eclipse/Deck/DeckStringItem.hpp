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

#ifndef DECKSTRINGITEM_HPP
#define DECKSTRINGITEM_HPP

#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>

namespace Opm {

    class DeckStringItem : public DeckItem {
    public:

        DeckStringItem(std::string name) : DeckItem(name) {
        }
        std::string getString(size_t index) const;
        const std::vector<std::string>& getStringData() const;

        void push_back(std::deque<std::string> data, size_t items);
        void push_back(std::deque<std::string> data);
        void push_back(std::string value);
        void push_backDefault(std::string value);
        void push_backMultiple(std::string value, size_t numItems);
        
        size_t size() const;
    private:
        std::vector<std::string> m_data;
    };

    typedef std::shared_ptr<DeckStringItem> DeckStringItemPtr;
    typedef std::shared_ptr<const DeckStringItem> DeckStringItemConstPtr;
}
#endif  

