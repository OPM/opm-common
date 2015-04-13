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



#include <iostream>
#include <string>
#include <memory>

#include <boost/iterator/iterator_facade.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>

namespace Opm {

class Section : public Deck
    {
    public:
        Section(DeckConstPtr deck, const std::string& startKeyword);
        const std::string& name() const;
        size_t count(const std::string& keyword) const;

        static bool hasSCHEDULE(DeckConstPtr deck) { return hasSection( deck , "SCHEDULE" ); }
        static bool hasSOLUTION(DeckConstPtr deck) { return hasSection( deck , "SOLUTION" ); }
        static bool hasREGIONS(DeckConstPtr deck) { return hasSection( deck , "REGIONS" ); }
        static bool hasPROPS(DeckConstPtr deck) { return hasSection( deck , "PROPS" ); }
        static bool hasEDIT(DeckConstPtr deck) { return hasSection( deck , "EDIT" ); }
        static bool hasGRID(DeckConstPtr deck) { return hasSection( deck , "GRID" ); }
        static bool hasRUNSPEC(DeckConstPtr deck) { return hasSection( deck , "RUNSPEC" ); }

        // returns whether the deck has all mandatory sections and if all sections are in
        // the right order
        static bool checkSectionTopology(DeckConstPtr deck,
                                         bool ensureKeywordSectionAffiliation = false);

    private:
        std::string m_name;
        static bool isSectionDelimiter(const std::string& keywordName);
        static bool hasSection(DeckConstPtr deck, const std::string& startKeyword);
        void populateSection(DeckConstPtr deck, const std::string& startKeyword);
    };

    typedef std::shared_ptr<Section> SectionPtr;
    typedef std::shared_ptr<const Section> SectionConstPtr;

    class RUNSPECSection : public Section {
    public:
        RUNSPECSection(DeckConstPtr deck) : Section(deck, "RUNSPEC") {}
    };

    class GRIDSection : public Section {
    public:
        GRIDSection(DeckConstPtr deck) : Section(deck, "GRID") {}
    };

    class EDITSection : public Section {
    public:
        EDITSection(DeckConstPtr deck) : Section(deck, "EDIT") {}
    };

    class PROPSSection : public Section {
    public:
        PROPSSection(DeckConstPtr deck) : Section(deck, "PROPS") {}
    };

    class REGIONSSection : public Section {
    public:
        REGIONSSection(DeckConstPtr deck) : Section(deck, "REGIONS") {}
    };

    class SOLUTIONSection : public Section {
    public:
        SOLUTIONSection(DeckConstPtr deck) : Section(deck, "SOLUTION") {}
    };

    class SCHEDULESection : public Section {
    public:
        SCHEDULESection(DeckConstPtr deck) : Section(deck, "SCHEDULE") {}
    };
}

#endif // SECTION_HPP
