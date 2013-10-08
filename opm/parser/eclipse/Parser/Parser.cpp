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
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>


namespace Opm {

    Parser::Parser(bool addDefault) {
        if (addDefault)
            addDefaultKeywords();
    }

    DeckPtr Parser::parse(const std::string &dataFile) const {
        return parse(dataFile, true);
    }


    /*
     About INCLUDE: Observe that the ECLIPSE parser is slightly unlogical
     when it comes to nested includes; the path to an included file is always
     interpreted relative to the filesystem location of the DATA file, and
     not the location of the file issuing the INCLUDE command. That behaviour
     is retained in the current implementation.
     */

    DeckPtr Parser::parse(const std::string &dataFileName, bool strictParsing) const {
        DeckPtr deck(new Deck());
        boost::filesystem::path dataFile(dataFileName);
        boost::filesystem::path rootPath;

        if (dataFile.is_absolute())
            rootPath = dataFile.parent_path();
        else
            rootPath = boost::filesystem::current_path() / dataFile.parent_path();

        parseFile(deck, dataFile, rootPath , strictParsing);
        return deck;
    }


    size_t Parser::size() const {
        return m_parserKeywords.size();
    }

    void Parser::addKeyword(ParserKeywordConstPtr parserKeyword) {
        m_parserKeywords.insert(std::make_pair(parserKeyword->getName(), parserKeyword));
    }

    bool Parser::hasKeyword(const std::string& keyword) const {
        return m_parserKeywords.find(keyword) != m_parserKeywords.end();
    }

    ParserKeywordConstPtr Parser::getKeyword(const std::string& keyword) const {
        if (hasKeyword(keyword)) {
            return m_parserKeywords.at(keyword);
        }
        else
            throw std::invalid_argument("Keyword: " + keyword + " does not exist");
    }



    void Parser::parseFile(DeckPtr deck, const boost::filesystem::path& file, const boost::filesystem::path& rootPath, bool parseStrict) const {
        bool verbose = false;
        std::ifstream inputstream;
        size_t lineNR = 0;
        inputstream.open(file.string().c_str());

        if (inputstream) {
            RawKeywordPtr rawKeyword;

            while (tryParseKeyword(deck, file.string() , lineNR , inputstream, rawKeyword, parseStrict)) {
                if (rawKeyword->getKeywordName() == Opm::RawConsts::include) {
                    RawRecordConstPtr firstRecord = rawKeyword->getRecord(0);
                    std::string includeFileString = firstRecord->getItem(0);
                    boost::filesystem::path includeFile(includeFileString);

                    if (includeFile.is_relative())
                        includeFile = rootPath / includeFile;

                    if (verbose)
                        std::cout << rawKeyword->getKeywordName() << "  " << includeFile << std::endl;

                    parseFile(deck, includeFile, rootPath , parseStrict);
                } else {
                    if (verbose)
                        std::cout << rawKeyword->getKeywordName() << std::endl;

                    if (hasKeyword(rawKeyword->getKeywordName())) {
                        ParserKeywordConstPtr parserKeyword = m_parserKeywords.at(rawKeyword->getKeywordName());
                        DeckKeywordConstPtr deckKeyword = parserKeyword->parse(rawKeyword);
                        deck->addKeyword(deckKeyword);
                    } else {
                        DeckKeywordPtr deckKeyword(new DeckKeyword(rawKeyword->getKeywordName(), false));
                        deck->addKeyword(deckKeyword);
                    }
                }
                rawKeyword.reset();
            }

            inputstream.close();
        } else
            throw std::invalid_argument("Failed to open file: " + file.string());
    }


    void Parser::loadKeywords(const Json::JsonObject& jsonKeywords) {
        if (jsonKeywords.is_array()) {
            for (size_t index = 0; index < jsonKeywords.size(); index++) {
                Json::JsonObject jsonKeyword = jsonKeywords.get_array_item(index);
                ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));

                addKeyword(parserKeyword);
            }
        } else
            throw std::invalid_argument("Input JSON object is not an array");
    }

    RawKeywordPtr Parser::createRawKeyword(const DeckConstPtr deck, const std::string& filename , size_t lineNR , const std::string& keywordString, bool strictParsing) const {
        if (hasKeyword(keywordString)) {
            ParserKeywordConstPtr parserKeyword = m_parserKeywords.find(keywordString)->second;
            if (parserKeyword->getSizeType() == SLASH_TERMINATED)
                return RawKeywordPtr(new RawKeyword(keywordString , filename , lineNR));
            else {
                size_t targetSize;

                if (parserKeyword->hasFixedSize())
                    targetSize = parserKeyword->getFixedSize();
                else {
                    const std::pair<std::string, std::string> sizeKeyword = parserKeyword->getSizeDefinitionPair();
                    DeckKeywordConstPtr sizeDefinitionKeyword = deck->getKeyword(sizeKeyword.first);
                    DeckItemConstPtr sizeDefinitionItem;
                    {
                        DeckRecordConstPtr record = sizeDefinitionKeyword->getRecord(0);
                        sizeDefinitionItem = record->getItem(sizeKeyword.second);
                    }
                    targetSize = sizeDefinitionItem->getInt(0);
                }
                return RawKeywordPtr(new RawKeyword(keywordString, filename , lineNR , targetSize , parserKeyword->isTableCollection()));
            }
        } else {
            if (strictParsing) {
                throw std::invalid_argument("Keyword " + keywordString + " not recognized ");
            } else {
                return RawKeywordPtr(new RawKeyword(keywordString, filename , lineNR , 0));
            }
        }
    }

    bool Parser::tryParseKeyword(const DeckConstPtr deck, const std::string& filename , size_t& lineNR , std::ifstream& inputstream, RawKeywordPtr& rawKeyword, bool strictParsing) const {
        std::string line;
        
        while (std::getline(inputstream, line)) {
            std::string keywordString;
            lineNR++;

            if (rawKeyword == NULL) {
                if (RawKeyword::tryParseKeyword(line, keywordString)) {
                    rawKeyword = createRawKeyword(deck, filename , lineNR , keywordString, strictParsing);
                }
            } else {
                if (RawKeyword::useLine(line)) {
                    rawKeyword->addRawRecordString(line);
                }
            }

            if (rawKeyword != NULL && rawKeyword->isFinished())
                return true;
        }

        return false;
    }

    bool Parser::loadKeywordFromFile(const boost::filesystem::path& configFile) {

        try {
            Json::JsonObject jsonKeyword(configFile);
            ParserKeywordConstPtr parserKeyword(new ParserKeyword(jsonKeyword));
            addKeyword(parserKeyword);
            return true;
        } 
        catch (...) {
            return false;
        }
    }


    void Parser::loadKeywordsFromDirectory(const boost::filesystem::path& directory, bool recursive, bool onlyALLCAPS8) {
        if (!boost::filesystem::exists(directory))
            throw std::invalid_argument("Directory: " + directory.string() + " does not exist.");
        else {
            boost::filesystem::directory_iterator end;
            for (boost::filesystem::directory_iterator iter(directory); iter != end; iter++) {
                if (boost::filesystem::is_directory(*iter)) {
                    if (recursive)
                        loadKeywordsFromDirectory(*iter, recursive, onlyALLCAPS8);
                } else {
                    if (ParserKeyword::validName(iter->path().filename().string()) || !onlyALLCAPS8) {
                        if (!loadKeywordFromFile(*iter))
                            std::cerr << "** Warning: failed to load keyword from file:" << iter->path() << std::endl;
                    }
                }
            }
        }
    }

} // namespace Opm
