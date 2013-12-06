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

#ifndef PARSER_H
#define PARSER_H
#include <string>
#include <map>
#include <fstream>
#include <memory>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>

namespace Opm {

    struct ParserState;

    /// The hub of the parsing process.
    /// An input file in the eclipse data format is specified, several steps of parsing is performed
    /// and the semantically parsed result is returned.

    class Parser {
    public:
        Parser(bool addDefault = true);

        /// The starting point of the parsing process. The supplied file is parsed, and the resulting Deck is returned.
        DeckPtr parse(const std::string &dataFile) const;
        DeckPtr parse(const std::string &dataFile, bool strictParsing) const;

        /// Method to add ParserKeyword instances, these holding type and size information about the keywords and their data.
        void addKeyword(ParserKeywordConstPtr parserKeyword);
        bool dropKeyword(const std::string& keyword);
        bool canParseKeyword( const std::string& keyword) const;
        ParserKeywordConstPtr getKeyword(const std::string& keyword) const;

        void loadKeywords(const Json::JsonObject& jsonKeywords);
        bool loadKeywordFromFile(const boost::filesystem::path& configFile);

        void loadKeywordsFromDirectory(const boost::filesystem::path& directory , bool recursive = true, bool onlyALLCAPS8Files = true);
        size_t size() const;
    private:
        std::map<std::string, ParserKeywordConstPtr> m_parserKeywords;
        std::map<std::string, ParserKeywordConstPtr> m_wildCardKeywords;

        bool hasKeyword(const std::string& keyword) const;
        bool hasWildCardKeyword(const std::string& keyword) const;
        ParserKeywordConstPtr matchingKeyword(const std::string& keyword) const;

        bool tryParseKeyword(std::shared_ptr<ParserState> parserState) const;
        void parseFile(std::shared_ptr<ParserState> parserState) const;
        RawKeywordPtr createRawKeyword(const std::string& keywordString, std::shared_ptr<ParserState> parserState) const;
        void addDefaultKeywords();

        boost::filesystem::path getRootPathFromFile(const boost::filesystem::path &inputDataFile) const;
    };


    typedef std::shared_ptr<Parser> ParserPtr;
    typedef std::shared_ptr<const Parser> ParserConstPtr;
} // namespace Opm
#endif  /* PARSER_H */

