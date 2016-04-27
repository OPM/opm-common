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

#include <cctype>
#include <fstream>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <opm/json/JsonObject.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/RawDeck/RawEnums.hpp>
#include <opm/parser/eclipse/RawDeck/RawKeyword.hpp>
#include <opm/parser/eclipse/Utility/Stringview.hpp>

namespace Opm {

    template< typename Itr >
    static inline Itr find_comment( Itr begin, Itr end ) {

        auto itr = std::find( begin, end, '-' );
        for( ; itr != end; itr = std::find( itr + 1, end, '-' ) )
            if( (itr + 1) != end &&  *( itr + 1 ) == '-' ) return itr;

        return end;
    }

    template< typename Itr, typename Term >
    static inline Itr strip_after( Itr begin, Itr end, Term terminator ) {

        auto pos = terminator( begin, end );

        if( pos == end ) return end;

        auto qbegin = std::find_if( begin, end, RawConsts::is_quote );

        if( qbegin == end || qbegin > pos )
            return pos;

        auto qend = std::find( qbegin + 1, end, *qbegin );

        // Quotes are not balanced - probably an error?!
        if( qend == end ) return end;

        return strip_after( qend + 1, end, terminator );
    }

    /**
       This function will return a copy of the input string where all
       characters following '--' are removed. The copy is a view and relies on
       the source string to remain alive. The function handles quoting with
       single quotes and double quotes:

         ABC --Comment                =>  ABC
         ABC '--Comment1' --Comment2  =>  ABC '--Comment1'
         ABC "-- Not balanced quote?  =>  ABC "-- Not balanced quote?
    */
    static inline string_view strip_comments( string_view str ) {
        return { str.begin(), strip_after( str.begin(), str.end(),
                find_comment< string_view::const_iterator > ) };
    }

    /* stripComments only exists so that the unit tests can verify it.
     * strip_comment is the actual (internal) implementation
     */
    std::string Parser::stripComments( const std::string& str ) {
        return { str.begin(), strip_after( str.begin(), str.end(),
                find_comment< std::string::const_iterator > ) };
    }

    template< typename Itr >
    static inline Itr trim_left( Itr begin, Itr end ) {

        return std::find_if_not( begin, end, RawConsts::is_separator );
    }

    template< typename Itr >
    static inline Itr trim_right( Itr begin, Itr end ) {

        std::reverse_iterator< Itr > rbegin( end );
        std::reverse_iterator< Itr > rend( begin );

        return std::find_if_not( rbegin, rend, RawConsts::is_separator ).base();
    }

    static inline string_view trim( string_view str ) {
        auto fst = trim_left( str.begin(), str.end() );
        auto lst = trim_right( fst, str.end() );
        return { fst, lst };
    }

    static inline string_view strip_slash( string_view view ) {
        using itr = string_view::const_iterator;
        const auto term = []( itr begin, itr end ) {
            return std::find( begin, end, '/' );
        };

        auto begin = view.begin();
        auto end = view.end();
        auto slash = strip_after( begin, end, term );

        /* we want to preserve terminating slashes */
        if( slash != end ) ++slash;

        return { begin, slash };
    }

    static inline bool getline( string_view& input, string_view& line ) {
        if( input.empty() ) return false;

        auto end = std::find( input.begin(), input.end(), '\n' );

        line = string_view( input.begin(), end );
        const auto mod = end == input.end() ? 0 : 1;
        input = string_view( end + mod, input.end() );
        return true;
    }

    /*
     * Read the input file and remove everything that isn't interesting data,
     * including stripping comments, removing leading/trailing whitespaces and
     * everything after (terminating) slashes
     */
    static inline std::string clean( const std::string& str ) {
        std::string dst;
        dst.reserve( str.size() );

        string_view input( str ), line;
        while( getline( input, line ) ) {
            line = trim( strip_slash( strip_comments( line ) ) );

            //if( line.begin() == line.end() ) continue;

            dst.append( line.begin(), line.end() );
            dst.append( 1, '\n' );
        }

        return dst;
    }

    const std::string emptystr = "";

    struct file {
        file( boost::filesystem::path p, const std::string& in ) :
            input( in ), path( p )
        {}

        string_view input;
        size_t lineNR = 0;
        boost::filesystem::path path;
    };

    class InputStack {
        public:
            bool empty() const { return this->file_stack.empty(); }
            file& peek() { return *this->file_stack.back(); }
            const file& peek() const { return *this->file_stack.back(); }
            void pop() { this->file_stack.pop_back(); };
            void push( std::string&& input, boost::filesystem::path p = "" );

        private:
            std::list< std::string > string_storage;
            std::list< file > file_storage;
            std::vector< file* > file_stack;
    };

    void InputStack::push( std::string&& input, boost::filesystem::path p ) {
        this->string_storage.push_back( std::move( input ) );
        this->file_storage.emplace_back( p, this->string_storage.back() );
        this->file_stack.push_back( &this->file_storage.back() );
    }

    struct ParserState {
        public:
            ParserState( const ParseContext& );
            void loadString( const std::string& );
            void loadFile( const boost::filesystem::path& );
            void openRootFile( const boost::filesystem::path& );
            void handleRandomText(const string_view& ) const;

            const boost::filesystem::path& current_path() const;
            size_t line() const;

            bool done() const;
            string_view getline();

            std::map< std::string, std::string > pathMap;
            boost::filesystem::path rootPath;
            Deck * deck;
            RawKeywordPtr rawKeyword;
            string_view nextKeyword = emptystr;
            InputStack input_stack;

            const ParseContext& parseContext;
    };


    const boost::filesystem::path& ParserState::current_path() const {
        return this->input_stack.peek().path;
    }

    size_t ParserState::line() const {
        return this->input_stack.peek().lineNR;
    }

    bool ParserState::done() const {

        while( !this->input_stack.empty() &&
                this->input_stack.peek().input.empty() )
            const_cast< ParserState* >( this )->input_stack.pop();

        return this->input_stack.empty();
    }

    string_view ParserState::getline() {
        string_view line;

        Opm::getline( this->input_stack.peek().input, line );
        this->input_stack.peek().lineNR++;

        return line;
    }

    ParserState::ParserState(const ParseContext& __parseContext)
        : deck( new Deck() ),
        parseContext( __parseContext )
    {}

    void ParserState::loadString(const std::string& input) {
        this->input_stack.push( clean( input ) );
    }

    void ParserState::loadFile(const boost::filesystem::path& inputFile) {

        boost::filesystem::path inputFileCanonical;
        try {
            inputFileCanonical = boost::filesystem::canonical(inputFile);
        } catch (boost::filesystem::filesystem_error fs_error) {
            throw std::runtime_error(std::string("Parser::loadFile fails: ") + fs_error.what());
        }

        const auto closer = []( std::FILE* f ) { std::fclose( f ); };
        std::unique_ptr< std::FILE, decltype( closer ) > ufp(
                std::fopen( inputFileCanonical.string().c_str(), "rb" ),
                closer
                );

        // make sure the file we'd like to parse is readable
        if( !ufp ) {
            throw std::runtime_error(std::string("Input file '") +
                    inputFileCanonical.string() +
                    std::string("' is not readable"));
        }

        /*
         * read the input file C-style. This is done for performance
         * reasons, as streams are slow
         */
        auto* fp = ufp.get();
        std::string buffer;
        std::fseek( fp, 0, SEEK_END );
        buffer.resize( std::ftell( fp ) );
        std::rewind( fp );
        std::fread( &buffer[ 0 ], 1, buffer.size(), fp );

        this->input_stack.push( clean( buffer ), inputFileCanonical );
    }

    void ParserState::openRootFile( const boost::filesystem::path& inputFile) {
        this->loadFile( inputFile );
        this->deck->setDataFile( inputFile.string() );
        const boost::filesystem::path& inputFileCanonical = boost::filesystem::canonical(inputFile);
        rootPath = inputFileCanonical.parent_path();
    }

    /*
       We have encountered 'random' characters in the input file which
       are not correctly formatted as a keyword heading, and not part
       of the data section of any keyword.
       */

    void ParserState::handleRandomText(const string_view& keywordString ) const {
        std::string errorKey;
        std::stringstream msg;
        std::string trimmedCopy = keywordString.string();

        if (trimmedCopy == "/") {
            errorKey = ParseContext::PARSE_RANDOM_SLASH;
            msg << "Extra '/' detected at: "
                << this->current_path()
                << ":" << this->line();
        } else {
            errorKey = ParseContext::PARSE_RANDOM_TEXT;
            msg << "String \'" << keywordString
                << "\' not formatted/recognized as valid keyword at: "
                << this->current_path()
                << ":" << this->line();
        }

        parseContext.handleError( errorKey , msg.str() );
    }

    std::shared_ptr< RawKeyword > createRawKeyword( const string_view& kw, ParserState& parserState, const Parser& parser ) {
        auto keywordString = ParserKeyword::getDeckName( kw );

        if( !parser.isRecognizedKeyword( keywordString ) ) {
            if( ParserKeyword::validDeckName( keywordString ) ) {
                std::string msg = "Keyword " + keywordString + " not recognized.";
                parserState.parseContext.handleError( ParseContext::PARSE_UNKNOWN_KEYWORD, msg );
                return {};
            }

            parserState.handleRandomText( keywordString );
            return {};

        }

        const auto* parserKeyword = parser.getParserKeywordFromDeckName( keywordString );

        if( parserKeyword->getSizeType() == SLASH_TERMINATED || parserKeyword->getSizeType() == UNKNOWN) {

            const auto rawSizeType = parserKeyword->getSizeType() == SLASH_TERMINATED
                                   ? Raw::SLASH_TERMINATED
                                   : Raw::UNKNOWN;

            return std::make_shared< RawKeyword >( keywordString, rawSizeType,
                                                   parserState.current_path().string(),
                                                   parserState.line() );
        }

        if( parserKeyword->hasFixedSize() ) {
            return std::make_shared< RawKeyword >( keywordString,
                                                   parserState.current_path().string(),
                                                   parserState.line(),
                                                   parserKeyword->getFixedSize(),
                                                   parserKeyword->isTableCollection() );
        }

        const auto& sizeKeyword = parserKeyword->getSizeDefinitionPair();
        const auto* deck = parserState.deck;

        if( deck->hasKeyword(sizeKeyword.first ) ) {
            const auto& sizeDefinitionKeyword = deck->getKeyword(sizeKeyword.first);
            const auto& record = sizeDefinitionKeyword.getRecord(0);
            const auto targetSize = record.getItem( sizeKeyword.second ).get< int >( 0 );
            return std::make_shared< RawKeyword >( keywordString,
                                                   parserState.current_path().string(),
                                                   parserState.line(),
                                                   targetSize,
                                                   parserKeyword->isTableCollection() );
        }

        std::string msg = "Expected the kewyord: " + sizeKeyword.first
                        + " to infer the number of records in: " + keywordString;

        parserState.parseContext.handleError(ParseContext::PARSE_MISSING_DIMS_KEYWORD , msg );

        const auto* keyword = parser.getKeyword( sizeKeyword.first );
        const auto record = keyword->getRecord(0);
        const auto int_item = std::dynamic_pointer_cast<const ParserIntItem>( record->get( sizeKeyword.second ) );

        const auto targetSize = int_item->getDefault( );
        return std::make_shared< RawKeyword >( keywordString,
                                               parserState.current_path().string(),
                                               parserState.line(),
                                               targetSize,
                                               parserKeyword->isTableCollection() );
    }

    bool tryParseKeyword( ParserState& parserState, const Parser& parser ) {
        if (parserState.nextKeyword.length() > 0) {
            parserState.rawKeyword = createRawKeyword( parserState.nextKeyword, parserState, parser );
            parserState.nextKeyword = emptystr;
        }

        if (parserState.rawKeyword && parserState.rawKeyword->isFinished())
            return true;

        while( !parserState.done() ) {
            auto line = parserState.getline();

            // skip empty lines
            if (line.size() == 0)
                continue;

            std::string keywordString;

            if( parserState.rawKeyword == NULL ) {
                if( RawKeyword::isKeywordPrefix( line, keywordString ) ) {
                    parserState.rawKeyword = createRawKeyword( keywordString, parserState, parser );
                } else
                    /* We are looking at some random gibberish?! */
                    parserState.handleRandomText( line );
            } else {
                if (parserState.rawKeyword->getSizeType() == Raw::UNKNOWN) {
                    if( parser.isRecognizedKeyword( line ) ) {
                        parserState.rawKeyword->finalizeUnknownSize();
                        parserState.nextKeyword = line;
                        return true;
                    }
                }
                parserState.rawKeyword->addRawRecordString(line);
            }

            if (parserState.rawKeyword
                && parserState.rawKeyword->isFinished()
                && parserState.rawKeyword->getSizeType() != Raw::UNKNOWN)
            {
                return true;
            }
        }

        if (parserState.rawKeyword
            && parserState.rawKeyword->getSizeType() == Raw::UNKNOWN)
        {
            parserState.rawKeyword->finalizeUnknownSize();
        }

        return false;
    }


    static boost::filesystem::path getIncludeFilePath(ParserState& parserState, std::string path)
    {
        const std::string pathKeywordPrefix("$");
        const std::string validPathNameCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");

        size_t positionOfPathName = path.find(pathKeywordPrefix);

        if ( positionOfPathName != std::string::npos) {
            std::string stringStartingAtPathName = path.substr(positionOfPathName+1);
            size_t cutOffPosition = stringStartingAtPathName.find_first_not_of(validPathNameCharacters);
            std::string stringToFind = stringStartingAtPathName.substr(0, cutOffPosition);
            std::string stringToReplace = parserState.pathMap.at(stringToFind);
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


    /*
     About INCLUDE: Observe that the ECLIPSE parser is slightly unlogical
     when it comes to nested includes; the path to an included file is always
     interpreted relative to the filesystem location of the DATA file, and
     not the location of the file issuing the INCLUDE command. That behaviour
     is retained in the current implementation.
     */

    Deck * Parser::newDeckFromFile(const std::string &dataFileName, const ParseContext& parseContext) const {
        std::shared_ptr<ParserState> parserState = std::make_shared<ParserState>(parseContext);
        parserState->openRootFile( dataFileName );
        parseState(parserState);
        applyUnitsToDeck(*parserState->deck);

        return parserState->deck;
    }

    Deck * Parser::newDeckFromString(const std::string &data, const ParseContext& parseContext) const {
        std::shared_ptr<ParserState> parserState = std::make_shared<ParserState>(parseContext);
        parserState->loadString( data );

        parseState(parserState);
        applyUnitsToDeck(*parserState->deck);

        return parserState->deck;
    }


    DeckPtr Parser::parseFile(const std::string &dataFileName, const ParseContext& parseContext) const {
        return std::shared_ptr<Deck>( newDeckFromFile( dataFileName , parseContext));
    }

    DeckPtr Parser::parseString(const std::string &data, const ParseContext& parseContext) const {
        return std::shared_ptr<Deck>( newDeckFromString( data , parseContext));
    }

    size_t Parser::size() const {
        return m_deckParserKeywords.size();
    }

    bool Parser::hasInternalKeyword(const std::string& internalKeywordName) const {
        return (m_internalParserKeywords.count(internalKeywordName) > 0);
    }

    const ParserKeyword* Parser::getParserKeywordFromInternalName(const std::string& internalKeywordName) const {
        return m_internalParserKeywords.at(internalKeywordName).get();
    }

    const ParserKeyword* Parser::matchingKeyword(const string_view& name) const {
        for (auto iter = m_wildCardKeywords.begin(); iter != m_wildCardKeywords.end(); ++iter) {
            if (iter->second->matches(name))
                return iter->second;
        }
        return nullptr;
    }

    bool Parser::hasWildCardKeyword(const std::string& internalKeywordName) const {
        return (m_wildCardKeywords.count(internalKeywordName) > 0);
    }

    bool Parser::isRecognizedKeyword(const string_view& name ) const {
        if( !ParserKeyword::validDeckName( name ) )
            return false;

        if( m_deckParserKeywords.count( name ) )
            return true;

        return matchingKeyword( name );
    }

void Parser::addParserKeyword( std::unique_ptr< const ParserKeyword >&& parserKeyword) {
    /* since string_view's don't own their memory, to update a value in the map
     * we must *delete* the previous entry and then re-insert the equivalent
     * new value. This is because the string_view borrows its memory from the
     * ParserKeyword, and when the ParserKeyword is replaced it will most
     * likely be free'd.
     */

    string_view name( parserKeyword->getName() );
    m_internalParserKeywords.erase( name );
    m_internalParserKeywords[ name ].swap( parserKeyword );
    auto* ptr = m_internalParserKeywords[ name ].get();

    for (auto nameIt = ptr->deckNamesBegin();
            nameIt != ptr->deckNamesEnd();
            ++nameIt)
    {
        string_view nm( *nameIt );
        m_deckParserKeywords.erase( nm );
        m_deckParserKeywords[ nm ] = ptr;
    }

    if (ptr->hasMatchRegex()) {
        m_wildCardKeywords.erase( name );
        m_wildCardKeywords[ name ] = ptr;
    }

}


void Parser::addParserKeyword(const Json::JsonObject& jsonKeyword) {
    addParserKeyword( std::unique_ptr< ParserKeyword >( new ParserKeyword( jsonKeyword ) ) );
}


const ParserKeyword* Parser::getKeyword( const std::string& name ) const {
    return getParserKeywordFromDeckName( string_view( name ) );
}

const ParserKeyword* Parser::getParserKeywordFromDeckName(const string_view& name ) const {
    auto candidate = m_deckParserKeywords.find( name );

    if( candidate != m_deckParserKeywords.end() ) return candidate->second;

    const auto* wildCardKeyword = matchingKeyword( name );

    if ( !wildCardKeyword )
        throw std::invalid_argument( "Do not have parser keyword for parsing: " + name );

    return wildCardKeyword;
}

std::vector<std::string> Parser::getAllDeckNames () const {
    std::vector<std::string> keywords;
    for (auto iterator = m_deckParserKeywords.begin(); iterator != m_deckParserKeywords.end(); iterator++) {
        keywords.push_back(iterator->first.string());
    }
    for (auto iterator = m_wildCardKeywords.begin(); iterator != m_wildCardKeywords.end(); iterator++) {
        keywords.push_back(iterator->first.string());
    }
    return keywords;
}

bool Parser::parseState(std::shared_ptr<ParserState> parserState) const {

    while( !parserState->done() ) {

        parserState->rawKeyword.reset();

        const bool streamOK = tryParseKeyword( *parserState, *this );
        if( !parserState->rawKeyword && !streamOK )
            continue;

        if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::end)
            return true;

        if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::endinclude) {
            parserState->input_stack.pop();
            continue;
        }

        if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::paths) {
            for( const auto& record : *parserState->rawKeyword ) {
                std::string pathName = readValueToken<std::string>(record.getItem(0));
                std::string pathValue = readValueToken<std::string>(record.getItem(1));
                parserState->pathMap.emplace( pathName, pathValue );
            }

            continue;
        }

        if (parserState->rawKeyword->getKeywordName() == Opm::RawConsts::include) {
            auto& firstRecord = parserState->rawKeyword->getFirstRecord( );
            std::string includeFileAsString = readValueToken<std::string>(firstRecord.getItem(0));
            boost::filesystem::path includeFile = getIncludeFilePath(*parserState, includeFileAsString);

            parserState->loadFile( includeFile );
            continue;
        }

        if (isRecognizedKeyword(parserState->rawKeyword->getKeywordName())) {
            const auto* parserKeyword = getParserKeywordFromDeckName(parserState->rawKeyword->getKeywordName());
            parserState->deck->addKeyword( parserKeyword->parse(parserState->parseContext , parserState->rawKeyword ) );
        } else {
            DeckKeyword deckKeyword(parserState->rawKeyword->getKeywordName(), false);
            const std::string msg = "The keyword " + parserState->rawKeyword->getKeywordName() + " is not recognized";
            deckKeyword.setLocation(parserState->rawKeyword->getFilename(),
                    parserState->rawKeyword->getLineNR());
            parserState->deck->addKeyword( std::move( deckKeyword ) );
            parserState->deck->getMessageContainer().warning( parserState->current_path().string(), msg, parserState->line() );
        }
    }

    return true;
}


    void Parser::loadKeywords(const Json::JsonObject& jsonKeywords) {
        if (jsonKeywords.is_array()) {
            for (size_t index = 0; index < jsonKeywords.size(); index++) {
                Json::JsonObject jsonKeyword = jsonKeywords.get_array_item(index);
                addParserKeyword( std::unique_ptr< ParserKeyword >( new ParserKeyword( jsonKeyword ) ) );
            }
        } else
            throw std::invalid_argument("Input JSON object is not an array");
    }

    bool Parser::loadKeywordFromFile(const boost::filesystem::path& configFile) {

        try {
            Json::JsonObject jsonKeyword(configFile);
            ParserKeywordConstPtr parserKeyword = std::make_shared<const ParserKeyword>(jsonKeyword);
            addParserKeyword( std::unique_ptr< ParserKeyword >( new ParserKeyword( jsonKeyword ) ) );
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
        for( auto& deckKeyword : deck ) {

            if( !isRecognizedKeyword( deckKeyword.name() ) ) continue;

            const auto* parserKeyword = getParserKeywordFromDeckName( deckKeyword.name() );
            if( !parserKeyword->hasDimension() ) continue;

            parserKeyword->applyUnitsToDeck(deck , deckKeyword);
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
            deck.getMessageContainer().warning(msg);
            return false;
        }

        bool deckValid = true;

        if( deck.getKeyword(0).name() != "RUNSPEC" ) {
            std::string msg = "The first keyword of a valid deck must be RUNSPEC\n";
            auto curKeyword = deck.getKeyword(0);
            deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
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
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                continue;
            }

            if (curSectionName == "RUNSPEC") {
                if (curKeywordName != "GRID") {
                    std::string msg =
                        "The RUNSPEC section must be followed by GRID instead of "+curKeywordName;
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "GRID") {
                if (curKeywordName != "EDIT" && curKeywordName != "PROPS") {
                    std::string msg =
                        "The GRID section must be followed by EDIT or PROPS instead of "+curKeywordName;
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "EDIT") {
                if (curKeywordName != "PROPS") {
                    std::string msg =
                        "The EDIT section must be followed by PROPS instead of "+curKeywordName;
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "PROPS") {
                if (curKeywordName != "REGIONS" && curKeywordName != "SOLUTION") {
                    std::string msg =
                        "The PROPS section must be followed by REGIONS or SOLUTION instead of "+curKeywordName;
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "REGIONS") {
                if (curKeywordName != "SOLUTION") {
                    std::string msg =
                        "The REGIONS section must be followed by SOLUTION instead of "+curKeywordName;
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SOLUTION") {
                if (curKeywordName != "SUMMARY" && curKeywordName != "SCHEDULE") {
                    std::string msg =
                        "The SOLUTION section must be followed by SUMMARY or SCHEDULE instead of "+curKeywordName;
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SUMMARY") {
                if (curKeywordName != "SCHEDULE") {
                    std::string msg =
                        "The SUMMARY section must be followed by SCHEDULE instead of "+curKeywordName;
                    deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                    deckValid = false;
                }

                curSectionName = curKeywordName;
            }
            else if (curSectionName == "SCHEDULE") {
                // schedule is the last section, so every section delimiter after it is wrong...
                std::string msg =
                    "The SCHEDULE section must be the last one ("
                    +curKeywordName+" specified after SCHEDULE)";
                deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
                deckValid = false;
            }
        }

        // SCHEDULE is the last section and it is mandatory, so make sure it is there
        if (curSectionName != "SCHEDULE") {
            const auto& curKeyword = deck.getKeyword(deck.size() - 1);
            std::string msg =
                "The last section of a valid deck must be SCHEDULE (is "+curSectionName+")";
            deck.getMessageContainer().warning(curKeyword.getFileName(), msg, curKeyword.getLineNumber());
            deckValid = false;
        }

        return deckValid;
    }

} // namespace Opm
