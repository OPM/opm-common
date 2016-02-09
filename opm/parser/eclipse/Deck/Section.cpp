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

#include <iostream>
#include <exception>
#include <algorithm>
#include <cassert>
#include <set>
#include <string>

#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>

namespace Opm {

    static bool isSectionDelimiter( const DeckKeyword& keyword ) {
        const auto& name = keyword.name();
        for( const auto& x : { "RUNSPEC", "GRID", "EDIT", "PROPS",
                               "REGIONS", "SOLUTION", "SUMMARY", "SCHEDULE" } )
            if( name == x ) return true;

        return false;
    }

    static std::pair< DeckView::const_iterator, DeckView::const_iterator >
    find_section( const Deck& deck, const std::string& keyword ) {

        const auto fn = [&keyword]( const DeckKeyword& kw ) {
            return kw.name() == keyword;
        };

        auto first = std::find_if( deck.begin(), deck.end(), fn );
        if( first == deck.end() )
            throw std::invalid_argument( std::string( "Deck requires a '" ) + keyword + "' section" );

        auto last = std::find_if( first + 1, deck.end(), &isSectionDelimiter );

        if( last != deck.end() && last->name() == keyword )
            throw std::invalid_argument( std::string( "Deck contains the '" ) + keyword + "' section multiple times" );

        return { first, last };
    }

    Section::Section( const Deck& deck, const std::string& section )
        : DeckView( find_section( deck, section ) ),
          section_name( section )
    {}

    const std::string& Section::name() const {
        return this->section_name;
    }

    bool Section::hasRUNSPEC(const Deck& deck) { return deck.hasKeyword( "RUNSPEC" ); }
    bool Section::hasGRID(const Deck& deck) { return deck.hasKeyword( "GRID" ); }
    bool Section::hasEDIT(const Deck& deck) { return deck.hasKeyword( "EDIT" ); }
    bool Section::hasPROPS(const Deck& deck) { return deck.hasKeyword( "PROPS" ); }
    bool Section::hasREGIONS(const Deck& deck) { return deck.hasKeyword( "REGIONS" ); }
    bool Section::hasSOLUTION(const Deck& deck) { return deck.hasKeyword( "SOLUTION" ); }
    bool Section::hasSUMMARY(const Deck& deck) { return deck.hasKeyword( "SUMMARY" ); }
    bool Section::hasSCHEDULE(const Deck& deck) { return deck.hasKeyword( "SCHEDULE" ); }

    bool Section::checkSectionTopology(const Deck& deck,
                                       bool ensureKeywordSectionAffiliation)
    {
        if( deck.size() == 0 ) {
            std::string msg = "empty decks are invalid\n";
            OpmLog::addMessage(Log::MessageType::Warning , msg);
            return false;
        }

        bool deckValid = true;

        if (deck->getKeyword(0)->name() != "RUNSPEC") {
            std::string msg = "The first keyword of a valid deck must be RUNSPEC\n";
            auto curKeyword = deck->getKeyword(0);
            OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
            deckValid = false;
        }

        std::string curSectionName = deck->getKeyword(0)->name();
        size_t curKwIdx = 1;
        for (; curKwIdx < deck->size(); ++curKwIdx) {
            const auto& curKeyword = deck->getKeyword(curKwIdx);
            const std::string& curKeywordName = curKeyword->name();

            if (!isSectionDelimiter(curKeywordName)) {
                if (!curKeyword->hasParserKeyword())
                    // ignore unknown keywords for now (i.e. they can appear in any section)
                    continue;

                const auto &parserKeyword = curKeyword->getParserKeyword();
                if (ensureKeywordSectionAffiliation && !parserKeyword->isValidSection(curSectionName)) {
                    std::string msg =
                        "The keyword '"+curKeywordName+"' is located in the '"+curSectionName
                        +"' section where it is invalid";

                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                continue;
            }

            if (curSectionName == "RUNSPEC") {
                if (curKeywordName != "GRID") {
                    std::string msg =
                        "The RUNSPEC section must be followed by GRID instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "GRID") {
                if (curKeywordName != "EDIT" && curKeywordName != "PROPS") {
                    std::string msg =
                        "The GRID section must be followed by EDIT or PROPS instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "EDIT") {
                if (curKeywordName != "PROPS") {
                    std::string msg =
                        "The EDIT section must be followed by PROPS instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "PROPS") {
                if (curKeywordName != "REGIONS" && curKeywordName != "SOLUTION") {
                    std::string msg =
                        "The PROPS section must be followed by REGIONS or SOLUTION instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "REGIONS") {
                if (curKeywordName != "SOLUTION") {
                    std::string msg =
                        "The REGIONS section must be followed by SOLUTION instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SOLUTION") {
                if (curKeywordName != "SUMMARY" && curKeywordName != "SCHEDULE") {
                    std::string msg =
                        "The SOLUTION section must be followed by SUMMARY or SCHEDULE instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SUMMARY") {
                if (curKeywordName != "SCHEDULE") {
                    std::string msg =
                        "The SUMMARY section must be followed by SCHEDULE instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SCHEDULE") {
                // schedule is the last section, so every section delimiter after it is wrong...
                std::string msg =
                    "The SCHEDULE section must be the last one ("
                    +curKeywordName+" specified after SCHEDULE)";
                OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(curKeyword->getFileName() , curKeyword->getLineNumber() , msg));
                deckValid = false;
            }
        }

        // SCHEDULE is the last section and it is mandatory, so make sure it is there
        if (curSectionName != "SCHEDULE") {
            const auto& curKeyword = deck->getKeyword(deck->size() - 1);
            std::string msg =
                "The last section of a valid deck must be SCHEDULE (is "+curSectionName+")";
            OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword->getFileName(), curKeyword->getLineNumber(), msg));
            deckValid = false;
        }

        return deckValid;
    }
}
