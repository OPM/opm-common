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

#include <string>

#include <opm/parser/eclipse/Deck/Deck.hpp>

namespace Opm {

    class UnitSystem;

class Section : public DeckView {
    public:
        using DeckView::const_iterator;

        Section( std::shared_ptr< const Deck > deck, const std::string& startKeyword );
        const std::string& name() const;

        static bool hasRUNSPEC( std::shared_ptr< const Deck > );
        static bool hasGRID( std::shared_ptr< const Deck > );
        static bool hasEDIT( std::shared_ptr< const Deck > );
        static bool hasPROPS( std::shared_ptr< const Deck > );
        static bool hasREGIONS( std::shared_ptr< const Deck > );
        static bool hasSOLUTION( std::shared_ptr< const Deck > );
        static bool hasSUMMARY( std::shared_ptr< const Deck > );
        static bool hasSCHEDULE( std::shared_ptr< const Deck > );

        // returns whether the deck has all mandatory sections and if all sections are in
        // the right order
        static bool checkSectionTopology(std::shared_ptr< const Deck > deck,
                                         bool ensureKeywordSectionAffiliation = false);

    private:
        std::string section_name;

    };

    class RUNSPECSection : public Section {
    public:
        RUNSPECSection(std::shared_ptr< const Deck > deck) : Section(deck, "RUNSPEC") {}
    };

    class GRIDSection : public Section {
    public:
        GRIDSection(std::shared_ptr< const Deck > deck) : Section(deck, "GRID") {}
    };

    class EDITSection : public Section {
    public:
        EDITSection(std::shared_ptr< const Deck > deck) : Section(deck, "EDIT") {}
    };

    class PROPSSection : public Section {
    public:
        PROPSSection(std::shared_ptr< const Deck > deck) : Section(deck, "PROPS") {}
    };

    class REGIONSSection : public Section {
    public:
        REGIONSSection(std::shared_ptr< const Deck > deck) : Section(deck, "REGIONS") {}
    };

    class SOLUTIONSection : public Section {
    public:
        SOLUTIONSection(std::shared_ptr< const Deck > deck) : Section(deck, "SOLUTION") {}
    };

    class SUMMARYSection : public Section {
    public:
        SUMMARYSection(std::shared_ptr< const Deck > deck) : Section(deck, "SUMMARY") {}
    };
}

#endif // SECTION_HPP
