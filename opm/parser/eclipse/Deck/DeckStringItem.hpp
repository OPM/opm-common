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
#include <memory>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>

namespace Opm {

    class DeckStringItem : public DeckTypeItem< std::string > {
        public:
            using DeckTypeItem< std::string >::DeckTypeItem;

            const std::string& getString( size_t index ) const override;
            const std::vector< std::string >& getStringData() const override;

            std::string getTrimmedString( size_t index ) const;
    };

    typedef std::shared_ptr< DeckStringItem > DeckStringItemPtr;
    typedef std::shared_ptr< const DeckStringItem > DeckStringItemConstPtr;
}
#endif

