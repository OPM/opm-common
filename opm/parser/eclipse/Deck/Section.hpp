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

namespace Opm {

    class Section
    {
    public:
        Section(Deck& deck, const std::string& startKeyword, const std::vector<std::string>& stopKeywords );
        bool hasKeyword( const std::string& keyword ) const;

    private:
        KeywordContainer m_keywords;
        std::map<std::string, std::string> m_startStopKeywords;

        void populateKeywords(const Deck& deck, const std::string& startKeyword, const std::vector<std::string>& stopKeywords);
        void initializeStartStopKeywords();
    };

    class RUNSPECSection : public Section {
    public:
        RUNSPECSection(Deck& deck) : Section (deck, "RUNSPEC", std::vector<std::string>() = {"GRID"}) {}
    };

    class GRIDSection : public Section {
    public:
        GRIDSection(Deck& deck) : Section (deck, "GRID", std::vector<std::string>() = {"EDIT", "PROPS"}) {}
    };

    class EDITSection : public Section {
    public:
        EDITSection(Deck& deck) : Section (deck, "EDIT", std::vector<std::string>() = {"PROPS"}) {}
    };

    class PROPSSection : public Section {
    public:
        PROPSSection(Deck& deck) : Section (deck, "PROPS", std::vector<std::string>() = {"REGIONS", "SOLUTION"}) {}
    };

    class REGIONSSection : public Section {
    public:
        REGIONSSection(Deck& deck) : Section (deck, "REGIONS", std::vector<std::string>() = {"SOLUTION"}) {}
    };

    class SOLUTIONSection : public Section {
    public:
        SOLUTIONSection(Deck& deck) : Section (deck, "SOLUTION", std::vector<std::string>() = {"SUMMARY", "SCHEDULE"}) {}
    };
}

#endif // SECTION_HPP
