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

#ifndef DECK_HPP
#define DECK_HPP

#include <vector>
#include <memory>

#include <opm/parser/eclipse/Deck/KeywordContainer.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

    class Deck {
    public:
        Deck();
        bool hasKeyword( const std::string& keyword ) const;
        void addKeyword( DeckKeywordPtr keyword);
        DeckKeywordPtr getKeyword(const std::string& keyword , size_t index) const;
        DeckKeywordPtr getKeyword(const std::string& keyword) const;
        DeckKeywordPtr getKeyword(size_t index) const;

        size_t numKeywords(const std::string& keyword);
        const std::vector<DeckKeywordPtr>& getKeywordList(const std::string& keyword);
        size_t size() const;
        size_t numWarnings() const;
        void addWarning(const std::string& warningText , const std::string& filename , size_t lineNR);
        const std::pair<std::string , std::pair<std::string,size_t> >& getWarning( size_t index ) const;
        void initUnitSystem();
        
        std::shared_ptr<UnitSystem> getDefaultUnitSystem() const;
        std::shared_ptr<UnitSystem> getActiveUnitSystem()  const;

    private:
        KeywordContainerPtr m_keywords;
        std::shared_ptr<UnitSystem> m_defaultUnits;
        std::shared_ptr<UnitSystem> m_activeUnits;
        
        std::vector<std::pair<std::string , std::pair<std::string,size_t> > > m_warnings; //<"Warning Text" , <"Filename" , LineNR>>
    };

    typedef std::shared_ptr<Deck> DeckPtr;
    typedef std::shared_ptr<const Deck> DeckConstPtr;
}
#endif  /* DECK_HPP */

