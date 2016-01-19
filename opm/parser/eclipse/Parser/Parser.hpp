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

#ifndef OPM_PARSER_HPP
#define OPM_PARSER_HPP
#include <string>
#include <map>
#include <fstream>
#include <memory>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>

namespace Opm {

    struct ParserState;

    /// The hub of the parsing process.
    /// An input file in the eclipse data format is specified, several steps of parsing is performed
    /// and the semantically parsed result is returned.

    class Parser {
    public:
        Parser(bool addDefault = true);

        static std::string stripComments(const std::string& inputString);

        /// The starting point of the parsing process. The supplied file is parsed, and the resulting Deck is returned.
        DeckPtr parseFile(const std::string &dataFile, const ParseMode& parseMode) const;
        DeckPtr parseString(const std::string &data, const ParseMode& parseMode) const;
        DeckPtr parseStream(std::shared_ptr<std::istream> inputStream , const ParseMode& parseMode) const;

        Deck * newDeckFromFile(const std::string &dataFileName, const ParseMode& parseMode) const;
        Deck * newDeckFromString(const std::string &dataFileName, const ParseMode& parseMode) const;

        DeckPtr parseFile(const std::string &dataFile, bool strict = true) const;
        DeckPtr parseString(const std::string &data, bool strict = true) const;
        DeckPtr parseStream(std::shared_ptr<std::istream> inputStream , bool strict = true) const;

        /// Method to add ParserKeyword instances, these holding type and size information about the keywords and their data.
        void addParserKeyword(const Json::JsonObject& jsonKeyword);
        void addParserKeyword(ParserKeywordConstPtr parserKeyword);
        bool dropParserKeyword(const std::string& parserKeywordName);
        ParserKeywordConstPtr getKeyword(const std::string& name) const;

        bool isRecognizedKeyword( const std::string& deckKeywordName) const;
        ParserKeywordConstPtr getParserKeywordFromDeckName(const std::string& deckKeywordName) const;
        std::vector<std::string> getAllDeckNames () const;

        void loadKeywords(const Json::JsonObject& jsonKeywords);
        bool loadKeywordFromFile(const boost::filesystem::path& configFile);

        void loadKeywordsFromDirectory(const boost::filesystem::path& directory , bool recursive = true);
        void applyUnitsToDeck(Deck& deck) const;

        /*!
         * \brief Returns the approximate number of recognized keywords in decks
         *
         * This is an approximate number because regular expresions are disconsidered.
         */
        size_t size() const;

        /*!
         * \brief Returns whether the parser knows about an keyword with a given internal
         *        name.
         */
        bool hasInternalKeyword(const std::string& internalKeywordName) const;

        /*!
         * \brief Retrieve a ParserKeyword object given an internal keyword name.
         */
        ParserKeywordConstPtr getParserKeywordFromInternalName(const std::string& internalKeywordName) const;


        template <class T>
        void addKeyword() {
            addParserKeyword( std::make_shared<T>());
        }


    private:
        // associative map of the parser internal name and the corresponding ParserKeyword object
        std::map<std::string, ParserKeywordConstPtr> m_internalParserKeywords;
        // associative map of deck names and the corresponding ParserKeyword object
        std::map<std::string, ParserKeywordConstPtr> m_deckParserKeywords;
        // associative map of the parser internal names and the corresponding
        // ParserKeyword object for keywords which match a regular expression
        std::map<std::string, ParserKeywordConstPtr> m_wildCardKeywords;

        bool hasWildCardKeyword(const std::string& keyword) const;
        ParserKeywordConstPtr matchingKeyword(const std::string& keyword) const;

        bool tryParseKeyword(std::shared_ptr<ParserState> parserState) const;
        bool parseState(std::shared_ptr<ParserState> parserState) const;
        RawKeywordPtr createRawKeyword(const std::string& keywordString, std::shared_ptr<ParserState> parserState) const;
        void addDefaultKeywords();

        boost::filesystem::path getIncludeFilePath(std::shared_ptr<ParserState> parserState, std::string path) const;
        std::string doSpecialHandlingForTitleKeyword(std::string line, std::shared_ptr<ParserState> parserState) const;
    };


    typedef std::shared_ptr<Parser> ParserPtr;
    typedef std::shared_ptr<const Parser> ParserConstPtr;
} // namespace Opm
#endif  /* PARSER_H */

