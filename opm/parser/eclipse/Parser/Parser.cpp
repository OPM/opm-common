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

#include <fstream>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>
#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/RawDeck/RawEnums.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>

namespace Opm {


    struct ParserState {
        const ParseMode& parseMode;
        Deck * deck;
        boost::filesystem::path dataFile;
        boost::filesystem::path rootPath;
        std::map<std::string, std::string> pathMap;
        size_t lineNR;
        std::shared_ptr<std::istream> inputstream;
        RawKeywordPtr rawKeyword;
        std::string nextKeyword;


        ParserState(const ParserState& parent)
            : parseMode( parent.parseMode )
        {
            deck = parent.deck;
            pathMap = parent.pathMap;
            rootPath = parent.rootPath;
        }

        ParserState(const ParseMode& __parseMode)
            : parseMode( __parseMode )
        {
            deck = new Deck();
            lineNR = 0;
        }

        std::shared_ptr<ParserState> includeState(boost::filesystem::path& filename) {
            std::shared_ptr<ParserState> childState = std::make_shared<ParserState>( *this );
            childState->openFile( filename );
            return childState;
        }


        void openString(const std::string& input) {
            dataFile = "";
            inputstream.reset(new std::istringstream(input));
        }


        void openStream(std::shared_ptr<std::istream> istream) {
            dataFile = "";
            inputstream = istream;
        }


        void openFile(const boost::filesystem::path& inputFile) {
            std::ifstream *ifs = new std::ifstream(inputFile.string().c_str());

            // make sure the file we'd like to parse exists and is
            // readable
            if (!ifs->is_open()) {
                throw std::runtime_error(std::string("Input file '") +
                                         inputFile.string() +
                                         std::string("' does not exist or is not readable"));
            }

            inputstream.reset( ifs );
            dataFile = inputFile;
        }

        void openRootFile( const boost::filesystem::path& inputFile) {
            openFile( inputFile );
            if (inputFile.is_absolute())
                rootPath = inputFile.parent_path();
            else
                rootPath = boost::filesystem::current_path() / inputFile.parent_path();
        }

        /*
          We have encountered 'random' characters in the input file which
          are not correctly formatted as a keyword heading, and not part
          of the data section of any keyword.
        */

        void handleRandomText(const std::string& keywordString ) const {
            std::string errorKey;
            std::stringstream msg;
            std::string trimmedCopy = boost::algorithm::trim_copy( keywordString );

            if (trimmedCopy == "/") {
                errorKey = ParseMode::PARSE_RANDOM_SLASH;
                msg << "Extra '/' detected at: " << dataFile << ":" << lineNR;
            } else {
                errorKey = ParseMode::PARSE_RANDOM_TEXT;
                msg << "String \'" << keywordString << "\' not formatted/recognized as valid keyword at: " << dataFile << ":" << lineNR;
            }

            parseMode.handleError( errorKey , msg.str() );
        }

    };

    static boost::filesystem::path getIncludeFilePath(ParserState& parserState, std::string path)
    {
        const std::string pathKeywordPrefix("$");
        const std::string validPathNameCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");

        size_t positionOfPathName = path.find(pathKeywordPrefix);

        if ( positionOfPathName != std::string::npos) {
            std::string stringStartingAtPathName = path.substr(positionOfPathName+1);
            size_t cutOffPosition = stringStartingAtPathName.find_first_not_of(validPathNameCharacters);
            std::string stringToFind = stringStartingAtPathName.substr(0, cutOffPosition);
            std::string stringToReplace = parserState.pathMap[stringToFind];
            boost::replace_all(path, pathKeywordPrefix + stringToFind, stringToReplace);
        }

        boost::filesystem::path includeFilePath(path);

        if (includeFilePath.is_relative())
            includeFilePath = parserState.rootPath / includeFilePath;

        return includeFilePath;
    }


    Parser::Parser(bool addDefault) {
        if (addDefault)
            addDefaultKeywords();
    }


    /**
       This function will remove return a copy of the input string
       where all characters following '--' are removed. The function
       handles quoting with single quotes and double quotes:

         ABC --Comment                =>  ABC
         ABC '--Comment1' --Comment2  =>  ABC '--Comment1'
         ABC "-- Not balanced quote?  =>  ABC "-- Not balanced quote?

    */
    std::string Parser::stripComments(const std::string& inputString) {
        std::string uncommentedString;
        size_t offset = 0;

        while (true) {
            size_t commentPos = inputString.find("--" , offset);
            if (commentPos == std::string::npos) {
                uncommentedString = inputString;
                break;
            } else {
                size_t quoteStart = inputString.find_first_of("'\"" , offset);
                if (quoteStart == std::string::npos || quoteStart > commentPos) {
                    uncommentedString = inputString.substr(0 , commentPos );
                    break;
                } else {
                    char quoteChar = inputString[quoteStart];
                    size_t quoteEnd = inputString.find( quoteChar , quoteStart + 1);
                    if (quoteEnd == std::string::npos) {
                        // Quotes are not balanced - probably an error?!
                        uncommentedString = inputString;
                        break;
                    } else
                        offset = quoteEnd + 1;
                }
            }
        }

        return uncommentedString;
    }


    /*
     About INCLUDE: Observe that the ECLIPSE parser is slightly unlogical
     when it comes to nested includes; the path to an included file is always
     interpreted relative to the filesystem location of the DATA file, and
     not the location of the file issuing the INCLUDE command. That behaviour
     is retained in the current implementation.
     */

    Deck * Parser::newDeckFromFile(const std::string &dataFileName, const ParseMode& parseMode) const {
        std::shared_ptr<ParserState> parserState = std::make_shared<ParserState>(parseMode);
        parserState->openRootFile( dataFileName );
        parseState(parserState);
        applyUnitsToDeck(*parserState->deck);

        return parserState->deck;
    }

    Deck * Parser::newDeckFromString(const std::string &data, const ParseMode& parseMode) const {
        std::shared_ptr<ParserState> parserState = std::make_shared<ParserState>(parseMode);
        parserState->openString( data );

        parseState(parserState);
        applyUnitsToDeck(*parserState->deck);

        return parserState->deck;
    }


    DeckPtr Parser::parseFile(const std::string &dataFileName, const ParseMode& parseMode) const {
        return std::shared_ptr<Deck>( newDeckFromFile( dataFileName , parseMode));
    }

    DeckPtr Parser::parseString(const std::string &data, const ParseMode& parseMode) const {
        return std::shared_ptr<Deck>( newDeckFromString( data , parseMode));
    }

    DeckPtr Parser::parseStream(std::shared_ptr<std::istream> inputStream, const ParseMode& parseMode) const {
        std::shared_ptr<ParserState> parserState = std::make_shared<ParserState>(parseMode);
        parserState->openStream( inputStream );

        parseState(parserState);
        applyUnitsToDeck(*parserState->deck);

        return std::shared_ptr<Deck>( parserState->deck );
    }

    size_t Parser::size() const {
        return m_deckParserKeywords.size();
    }

    bool Parser::hasInternalKeyword(const std::string& internalKeywordName) const {
        return (m_internalParserKeywords.count(internalKeywordName) > 0);
    }

    ParserKeywordConstPtr Parser::getParserKeywordFromInternalName(const std::string& internalKeywordName) const {
        return m_internalParserKeywords.at(internalKeywordName);
    }

    ParserKeywordConstPtr Parser::matchingKeyword(const std::string& name) const {
        for (auto iter = m_wildCardKeywords.begin(); iter != m_wildCardKeywords.end(); ++iter) {
            if (iter->second->matches(name))
                return iter->second;
        }
        return ParserKeywordConstPtr();
    }

    bool Parser::hasWildCardKeyword(const std::string& internalKeywordName) const {
        return (m_wildCardKeywords.count(internalKeywordName) > 0);
    }

    bool Parser::isRecognizedKeyword(const std::string& deckKeywordName) const {
        if (!ParserKeyword::validDeckName(deckKeywordName)) {
            return false;
        }

        if (m_deckParserKeywords.count(deckKeywordName) > 0)
            return true;

        ParserKeywordConstPtr wildCardKeyword = matchingKeyword( deckKeywordName );
        return wildCardKeyword?true:false;
    }

    void Parser::addParserKeyword(ParserKeywordConstPtr parserKeyword) {
        m_internalParserKeywords[parserKeyword->getName()] = parserKeyword;

        for (auto nameIt = parserKeyword->deckNamesBegin();
             nameIt != parserKeyword->deckNamesEnd();
             ++nameIt)
        {
            m_deckParserKeywords[*nameIt] = parserKeyword;
        }

        if (parserKeyword->hasMatchRegex())
            m_wildCardKeywords[parserKeyword->getName()] = parserKeyword;
    }


    void Parser::addParserKeyword(const Json::JsonObject& jsonKeyword) {
        ParserKeywordConstPtr parserKeyword = std::make_shared<const ParserKeyword>(jsonKeyword);
        addParserKeyword(parserKeyword);
    }


    ParserKeywordConstPtr Parser::getKeyword(const std::string& name ) const {
        auto iter = m_deckParserKeywords.find( name );
        if (iter == m_deckParserKeywords.end())
            throw std::invalid_argument("Keyword not found");
        else
            return iter->second;
    }


    bool Parser::dropParserKeyword(const std::string& parserKeywordName) {
        // remove from the internal from the internal names
        bool erase = (m_internalParserKeywords.erase( parserKeywordName ) > 0);

        // remove keyword from the deck names map
        auto deckParserKeywordIt = m_deckParserKeywords.begin();
        while (deckParserKeywordIt != m_deckParserKeywords.end()) {
            if (deckParserKeywordIt->second->getName() == parserKeywordName)
                // note the post-increment of the iterator. this is required to keep the
                // iterator valid for while at the same time erasing it...
                m_deckParserKeywords.erase(deckParserKeywordIt++);
            else
                ++ deckParserKeywordIt;
        }

        // remove the keyword from the wildcard list
        m_wildCardKeywords.erase( parserKeywordName );

        return erase;
    }



    ParserKeywordConstPtr Parser::getParserKeywordFromDeckName(const std::string& deckKeywordName) const {
        if (m_deckParserKeywords.count(deckKeywordName)) {
            return m_deckParserKeywords.at(deckKeywordName);
        } else {
            ParserKeywordConstPtr wildCardKeyword = matchingKeyword( deckKeywordName );

            if (wildCardKeyword)
                return wildCardKeyword;
            else
                throw std::invalid_argument("Do not have parser keyword for parsing: " + deckKeywordName);
        }
    }

    std::vector<std::string> Parser::getAllDeckNames () const {
        std::vector<std::string> keywords;
        for (auto iterator = m_deckParserKeywords.begin(); iterator != m_deckParserKeywords.end(); iterator++) {
            keywords.push_back(iterator->first);
        }
        for (auto iterator = m_wildCardKeywords.begin(); iterator != m_wildCardKeywords.end(); iterator++) {
            keywords.push_back(iterator->first);
        }
        return keywords;
    }

    bool Parser::parseState(std::shared_ptr<ParserState> parserState) const {
        bool stopParsing = false;

        if (parserState->inputstream) {
            while (true) {
                bool streamOK = tryParseKeyword(parserState);
                if (parserState->rawKeyword) {
                    if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::end) {
                        stopParsing = true;
                        break;
                    }
                    else if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::endinclude) {
                        break;
                    }
                    else if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::paths) {
                        for (size_t i = 0; i < parserState->rawKeyword->size(); i++) {
                             RawRecordConstPtr record = parserState->rawKeyword->getRecord(i);
                             std::string pathName = readValueToken<std::string>(record->getItem(0));
                             std::string pathValue = readValueToken<std::string>(record->getItem(1));
                             parserState->pathMap.insert(std::pair<std::string, std::string>(pathName, pathValue));
                        }
                    }
                    else if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::include) {
                        RawRecordConstPtr firstRecord = parserState->rawKeyword->getRecord(0);
                        std::string includeFileAsString = readValueToken<std::string>(firstRecord->getItem(0));
                        boost::filesystem::path includeFile = getIncludeFilePath(*parserState, includeFileAsString);
                        std::shared_ptr<ParserState> newParserState = parserState->includeState( includeFile );


                        stopParsing = parseState(newParserState);
                        if (stopParsing) break;
                    } else {

                        if (isRecognizedKeyword(parserState->rawKeyword->getKeywordName())) {
                            ParserKeywordConstPtr parserKeyword = getParserKeywordFromDeckName(parserState->rawKeyword->getKeywordName());
                            parserState->deck->addKeyword( parserKeyword->parse(parserState->parseMode , parserState->rawKeyword ) );
                        } else {
                            DeckKeyword deckKeyword(parserState->rawKeyword->getKeywordName(), false);
                            const std::string msg = "The keyword " + parserState->rawKeyword->getKeywordName() + " is not recognized";
                            deckKeyword.setLocation(parserState->rawKeyword->getFilename(),
                                                     parserState->rawKeyword->getLineNR());
                            parserState->deck->addKeyword( std::move( deckKeyword ) );
                            OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage(parserState->dataFile.string() , parserState->lineNR , msg));
                        }
                    }
                    parserState->rawKeyword.reset();
                }
                if (!streamOK)
                    break;
            }
        } else
            throw std::invalid_argument("Failed to open file: " + parserState->dataFile.string());
        return stopParsing;
    }


    void Parser::loadKeywords(const Json::JsonObject& jsonKeywords) {
        if (jsonKeywords.is_array()) {
            for (size_t index = 0; index < jsonKeywords.size(); index++) {
                Json::JsonObject jsonKeyword = jsonKeywords.get_array_item(index);
                ParserKeywordConstPtr parserKeyword = std::make_shared<const ParserKeyword>(jsonKeyword);

                addParserKeyword(parserKeyword);
            }
        } else
            throw std::invalid_argument("Input JSON object is not an array");
    }

    RawKeywordPtr Parser::createRawKeyword(const std::string& initialLine, std::shared_ptr<ParserState> parserState) const {
        std::string keywordString = ParserKeyword::getDeckName(initialLine);

        if (isRecognizedKeyword(keywordString)) {
            ParserKeywordConstPtr parserKeyword = getParserKeywordFromDeckName( keywordString );

            if (parserKeyword->getSizeType() == SLASH_TERMINATED || parserKeyword->getSizeType() == UNKNOWN) {
                Raw::KeywordSizeEnum rawSizeType;
                if (parserKeyword->getSizeType() == SLASH_TERMINATED)
                    rawSizeType = Raw::SLASH_TERMINATED;
                else
                    rawSizeType = Raw::UNKNOWN;

                return RawKeywordPtr(new RawKeyword(keywordString , rawSizeType , parserState->dataFile.string(), parserState->lineNR));
            } else {
                size_t targetSize;

                if (parserKeyword->hasFixedSize())
                    targetSize = parserKeyword->getFixedSize();
                else {
                    const std::pair<std::string, std::string> sizeKeyword = parserKeyword->getSizeDefinitionPair();
                    const Deck * deck = parserState->deck;
                    if (deck->hasKeyword(sizeKeyword.first)) {
                        const auto& sizeDefinitionKeyword = deck->getKeyword(sizeKeyword.first);
                        const auto& record = sizeDefinitionKeyword.getRecord(0);
                        targetSize = record.getItem( sizeKeyword.second ).get< int >( 0 );
                    } else {
                        std::string msg = "Expected the kewyord: " + sizeKeyword.first + " to infer the number of records in: " + keywordString;
                        parserState->parseMode.handleError(ParseMode::PARSE_MISSING_DIMS_KEYWORD , msg );

                        auto keyword = getKeyword( sizeKeyword.first );
                        auto record = keyword->getRecord(0);
                        auto int_item = std::dynamic_pointer_cast<const ParserIntItem>( record->get( sizeKeyword.second ) );

                        targetSize = int_item->getDefault( );
                    }
                }
                return RawKeywordPtr(new RawKeyword(keywordString, parserState->dataFile.string() , parserState->lineNR , targetSize , parserKeyword->isTableCollection()));
            }
        } else {
            if (ParserKeyword::validDeckName(keywordString)) {
                std::string msg = "Keyword " + keywordString + " not recognized ";
                parserState->parseMode.handleError( ParseMode::PARSE_UNKNOWN_KEYWORD  , msg );
                return std::shared_ptr<RawKeyword>(  );
            } else {
                parserState->handleRandomText( keywordString );
                return std::shared_ptr<RawKeyword>(  );
            }
        }
    }




    std::string Parser::doSpecialHandlingForTitleKeyword(std::string line, std::shared_ptr<ParserState> parserState) const {
        if ((parserState->rawKeyword != NULL) && (parserState->rawKeyword->getKeywordName() == "TITLE"))
                line = line.append("/");
        return line;
    }

    bool Parser::tryParseKeyword(std::shared_ptr<ParserState> parserState) const {
        std::string line;

        if (parserState->nextKeyword.length() > 0) {
            parserState->rawKeyword = createRawKeyword(parserState->nextKeyword, parserState);
            parserState->nextKeyword = "";
        }

        if (parserState->rawKeyword && parserState->rawKeyword->isFinished())
            return true;

        while (std::getline(*parserState->inputstream, line)) {
            if (line.find("--") != std::string::npos)
                line = stripComments( line );

            boost::algorithm::trim_right(line); // Removing garbage (eg. \r)
            line = doSpecialHandlingForTitleKeyword(line, parserState);
            std::string keywordString;
            parserState->lineNR++;

            // skip empty lines
            if (line.size() == 0)
                continue;

            if (parserState->rawKeyword == NULL) {
                if (RawKeyword::isKeywordPrefix(line, keywordString)) {
                    parserState->rawKeyword = createRawKeyword(keywordString, parserState);
                } else
                    /* We are looking at some random gibberish?! */
                    parserState->handleRandomText( line );
            } else {
                if (parserState->rawKeyword->getSizeType() == Raw::UNKNOWN) {
                    if (isRecognizedKeyword(line)) {
                        parserState->rawKeyword->finalizeUnknownSize();
                        parserState->nextKeyword = line;
                        return true;
                    }
                }
                parserState->rawKeyword->addRawRecordString(line);
                line = "";
            }

            if (parserState->rawKeyword
                && parserState->rawKeyword->isFinished()
                && parserState->rawKeyword->getSizeType() != Raw::UNKNOWN)
            {
                return true;
            }
        }
        if (parserState->rawKeyword
            && parserState->rawKeyword->getSizeType() == Raw::UNKNOWN)
        {
            parserState->rawKeyword->finalizeUnknownSize();
        }

        return false;
    }

    bool Parser::loadKeywordFromFile(const boost::filesystem::path& configFile) {

        try {
            Json::JsonObject jsonKeyword(configFile);
            ParserKeywordConstPtr parserKeyword = std::make_shared<const ParserKeyword>(jsonKeyword);
            addParserKeyword(parserKeyword);
            return true;
        }
        catch (...) {
            return false;
        }
    }


    void Parser::loadKeywordsFromDirectory(const boost::filesystem::path& directory, bool recursive) {
        if (!boost::filesystem::exists(directory))
            throw std::invalid_argument("Directory: " + directory.string() + " does not exist.");
        else {
            boost::filesystem::directory_iterator end;
            for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
                if (boost::filesystem::is_directory(*iter)) {
                    if (recursive)
                        loadKeywordsFromDirectory(*iter, recursive);
                } else {
                    if (ParserKeyword::validInternalName(iter->path().filename().string())) {
                        if (!loadKeywordFromFile(*iter))
                            std::cerr << "** Warning: failed to load keyword from file:" << iter->path() << std::endl;
                    }
                }
            }
        }
    }


    void Parser::applyUnitsToDeck(Deck& deck) const {
        deck.initUnitSystem();
        for (size_t index=0; index < deck.size(); ++index) {
            auto& deckKeyword = deck.getKeyword( index );
            if (isRecognizedKeyword( deckKeyword.name())) {
                ParserKeywordConstPtr parserKeyword = getParserKeywordFromDeckName( deckKeyword.name() );
                if (parserKeyword->hasDimension()) {
                    parserKeyword->applyUnitsToDeck(deck , deckKeyword);
                }
            }
        }
    }

    static bool isSectionDelimiter( const DeckKeyword& keyword ) {
        const auto& name = keyword.name();
        for( const auto& x : { "RUNSPEC", "GRID", "EDIT", "PROPS",
                               "REGIONS", "SOLUTION", "SUMMARY", "SCHEDULE" } )
            if( name == x ) return true;

        return false;
    }

    bool Section::checkSectionTopology(const Deck& deck,
                                       const Parser& parser,
                                       bool ensureKeywordSectionAffiliation)
    {
        if( deck.size() == 0 ) {
            std::string msg = "empty decks are invalid\n";
            OpmLog::addMessage( Log::MessageType::Warning, msg );
            return false;
        }

        bool deckValid = true;

        if( deck.getKeyword(0).name() != "RUNSPEC" ) {
            std::string msg = "The first keyword of a valid deck must be RUNSPEC\n";
            auto curKeyword = deck.getKeyword(0);
            OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
            deckValid = false;
        }

        std::string curSectionName = deck.getKeyword(0).name();
        size_t curKwIdx = 1;
        for (; curKwIdx < deck.size(); ++curKwIdx) {
            const auto& curKeyword = deck.getKeyword(curKwIdx);
            const std::string& curKeywordName = curKeyword.name();

            if (!isSectionDelimiter( curKeyword )) {
                if( !parser.isRecognizedKeyword( curKeywordName ) )
                    // ignore unknown keywords for now (i.e. they can appear in any section)
                    continue;

                const auto& parserKeyword = parser.getParserKeywordFromDeckName( curKeywordName );
                if (ensureKeywordSectionAffiliation && !parserKeyword->isValidSection(curSectionName)) {
                    std::string msg =
                        "The keyword '"+curKeywordName+"' is located in the '"+curSectionName
                        +"' section where it is invalid";

                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                continue;
            }

            if (curSectionName == "RUNSPEC") {
                if (curKeywordName != "GRID") {
                    std::string msg =
                        "The RUNSPEC section must be followed by GRID instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "GRID") {
                if (curKeywordName != "EDIT" && curKeywordName != "PROPS") {
                    std::string msg =
                        "The GRID section must be followed by EDIT or PROPS instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "EDIT") {
                if (curKeywordName != "PROPS") {
                    std::string msg =
                        "The EDIT section must be followed by PROPS instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "PROPS") {
                if (curKeywordName != "REGIONS" && curKeywordName != "SOLUTION") {
                    std::string msg =
                        "The PROPS section must be followed by REGIONS or SOLUTION instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "REGIONS") {
                if (curKeywordName != "SOLUTION") {
                    std::string msg =
                        "The REGIONS section must be followed by SOLUTION instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SOLUTION") {
                if (curKeywordName != "SUMMARY" && curKeywordName != "SCHEDULE") {
                    std::string msg =
                        "The SOLUTION section must be followed by SUMMARY or SCHEDULE instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SUMMARY") {
                if (curKeywordName != "SCHEDULE") {
                    std::string msg =
                        "The SUMMARY section must be followed by SCHEDULE instead of "+curKeywordName;
                    OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SCHEDULE") {
                // schedule is the last section, so every section delimiter after it is wrong...
                std::string msg =
                    "The SCHEDULE section must be the last one ("
                    +curKeywordName+" specified after SCHEDULE)";
                OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
                deckValid = false;
            }
        }

        // SCHEDULE is the last section and it is mandatory, so make sure it is there
        if (curSectionName != "SCHEDULE") {
            const auto& curKeyword = deck.getKeyword(deck.size() - 1);
            std::string msg =
                "The last section of a valid deck must be SCHEDULE (is "+curSectionName+")";
            OpmLog::addMessage(Log::MessageType::Warning, Log::fileMessage(curKeyword.getFileName(), curKeyword.getLineNumber(), msg));
            deckValid = false;
        }

        return deckValid;
    }

} // namespace Opm
