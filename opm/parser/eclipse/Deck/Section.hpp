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

#ifndef SECTION_HPP
#define SECTION_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <boost/iterator/iterator_facade.hpp>

namespace Opm {

    class Section : public boost::iterator_facade<Section, DeckKeywordPtr, boost::forward_traversal_tag>
    {
    public:
        Section(DeckConstPtr deck, const std::string& startKeyword, const std::vector<std::string>& stopKeywords );
        bool hasKeyword( const std::string& keyword ) const;
        std::vector<DeckKeywordPtr>::iterator begin();
        std::vector<DeckKeywordPtr>::iterator end();
        DeckKeywordConstPtr getKeyword(const std::string& keyword, size_t index) const;
        DeckKeywordConstPtr getKeyword(const std::string& keyword) const;
        DeckKeywordConstPtr getKeyword(size_t index) const;
        const std::string& name() const;

        static bool hasSCHEDULE(DeckConstPtr deck) { return hasSection( deck , "SCHEDULE" ); }
        static bool hasSOLUTION(DeckConstPtr deck) { return hasSection( deck , "SOLUTION" ); }
        static bool hasREGIONS(DeckConstPtr deck) { return hasSection( deck , "REGIONS" ); }
        static bool hasPROPS(DeckConstPtr deck) { return hasSection( deck , "PROPS" ); }
        static bool hasEDIT(DeckConstPtr deck) { return hasSection( deck , "EDIT" ); }
        static bool hasGRID(DeckConstPtr deck) { return hasSection( deck , "GRID" ); }
        static bool hasRUNSPEC(DeckConstPtr deck) { return hasSection( deck , "RUNSPEC" ); }
    private:
        KeywordContainer m_keywords;
        std::string m_name;
        static bool hasSection(DeckConstPtr deck, const std::string& startKeyword);
        void populateKeywords(DeckConstPtr deck, const std::string& startKeyword, const std::vector<std::string>& stopKeywords);
    };

    typedef std::shared_ptr<Section> SectionPtr;
    typedef std::shared_ptr<const Section> SectionConstPtr;

    
    class RUNSPECSection : public Section {
    public:
        RUNSPECSection(DeckConstPtr deck) : Section (deck, "RUNSPEC", std::vector<std::string>() = {"GRID"}) {}
    };

    class GRIDSection : public Section {
    public:
        GRIDSection(DeckConstPtr deck) : Section (deck, "GRID", std::vector<std::string>() = {"EDIT", "PROPS"}) {}
    };

    class EDITSection : public Section {
    public:
        EDITSection(DeckConstPtr deck) : Section (deck, "EDIT", std::vector<std::string>() = {"PROPS"}) {}
    };

    class PROPSSection : public Section {
    public:
        PROPSSection(DeckConstPtr deck) : Section (deck, "PROPS", std::vector<std::string>() = {"REGIONS", "SOLUTION"}) {}
    };

    class REGIONSSection : public Section {
    public:
        REGIONSSection(DeckConstPtr deck) : Section (deck, "REGIONS", std::vector<std::string>() = {"SOLUTION"}) {}
    };

    class SOLUTIONSection : public Section {
    public:
        SOLUTIONSection(DeckConstPtr deck) : Section (deck, "SOLUTION", std::vector<std::string>() = {"SUMMARY", "SCHEDULE"}) {}
    };

    class SCHEDULESection : public Section {
    public:
        SCHEDULESection(DeckConstPtr deck) : Section (deck, "SCHEDULE", std::vector<std::string>() = {}) {}
    };
}

#endif // SECTION_HPP
