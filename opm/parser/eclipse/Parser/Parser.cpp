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
#include <opm/parser/eclipse/Logger/Logger.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>


namespace Opm {

    Parser::Parser() {
        populateDefaultKeywords();
    }

    Parser::Parser(const boost::filesystem::path& jsonFile) {
        initializeFromJsonFile( jsonFile );
    }


    DeckPtr Parser::parse(const std::string &dataFile) {
        DeckPtr deck(new Deck());

        parseFile( deck , dataFile );
        return deck;
    }


    void Parser::parseFile(DeckPtr deck , const std::string &file) {
        std::ifstream inputstream;
        inputstream.open( file.c_str() );
        
        if (inputstream) {
          RawKeywordPtr rawKeyword;
          
          while (tryParseKeyword(deck , inputstream , rawKeyword)) {
            if (rawKeyword->getKeywordName() == Opm::RawConsts::include) {
              boost::filesystem::path dataFolderPath = verifyValidInputPath(file);
              RawRecordConstPtr firstRecord = rawKeyword->getRecord(0);
              std::string includeFileString = firstRecord->getItem(0);
              boost::filesystem::path pathToIncludedFile(dataFolderPath);
              pathToIncludedFile /= includeFileString;
              
              parseFile( deck , pathToIncludedFile.string() );
            } else {
              ParserKeywordConstPtr parserKeyword = m_parserKeywords[rawKeyword->getKeywordName()];
              DeckKeywordConstPtr deckKeyword = parserKeyword->parse(rawKeyword);
              deck->addKeyword( deckKeyword );
            }
            rawKeyword.reset();
          }
          
          inputstream.close();
        } else
          throw std::invalid_argument("Failed to open file: " + file);
    }


    
    void Parser::addKeyword(ParserKeywordConstPtr parserKeyword) {
        m_parserKeywords.insert(std::make_pair(parserKeyword->getName(), parserKeyword));
    }

    
    void Parser::initializeFromJsonFile( const boost::filesystem::path& jsonFile ) {
        Json::JsonObject jsonConfig(jsonFile);
        if (jsonConfig.has_item("keywords")) {
            Json::JsonObject jsonKeywords = jsonConfig.get_item("keywords");
            loadKeywords( jsonKeywords );
        } else
            throw std::invalid_argument("Missing \"keywords\" section in config file: " + jsonFile.string());
    }


    void Parser::loadKeywords(const Json::JsonObject& jsonKeywords) {
        if (jsonKeywords.is_array()) {
            for (size_t index = 0; index < jsonKeywords.size(); index++) {
                Json::JsonObject jsonKeyword = jsonKeywords.get_array_item( index );
                ParserKeywordConstPtr parserKeyword( new ParserKeyword( jsonKeyword ));
                
                addKeyword( parserKeyword );
            }
        } else
            throw std::invalid_argument("Input JSON object is not an array");
    }


    bool Parser::hasKeyword(const std::string& keyword) const {
        return m_parserKeywords.find(keyword) != m_parserKeywords.end();
    }




    RawKeywordPtr Parser::newRawKeyword(const DeckConstPtr deck , const std::string& keywordString) {
        if (hasKeyword(keywordString)) {
            ParserKeywordConstPtr parserKeyword = m_parserKeywords.find(keywordString)->second;
            if (parserKeyword->getSizeType() == UNDEFINED) 
                return RawKeywordPtr(new RawKeyword(keywordString));
            else {
                size_t targetSize;
                
                if (parserKeyword->hasFixedSize())
                    targetSize = parserKeyword->getFixedSize();
                else {
                    const std::pair<std::string,std::string> sizeKeyword = parserKeyword->getSizePair();
                    DeckKeywordConstPtr deckKeyword = deck->getKeyword(sizeKeyword.first);
                    DeckRecordConstPtr deckRecord = deckKeyword->getRecord(0);
                    DeckItemConstPtr deckItem = deckRecord->getItem(sizeKeyword.second);
                    
                    targetSize = deckItem->getInt(0);
                }
                return RawKeywordPtr(new RawKeyword(keywordString , targetSize));
            }
        } else
            throw std::invalid_argument("Keyword " + keywordString + " not recognized ");
    }



    
    bool Parser::tryParseKeyword(const DeckConstPtr deck ,  std::ifstream& inputstream , RawKeywordPtr& rawKeyword) {        
        std::string line;

        while (std::getline(inputstream, line)) {
            std::string keywordString;

            if (rawKeyword == NULL) {
                if (RawKeyword::tryParseKeyword(line, keywordString)) 
                    rawKeyword = newRawKeyword( deck , keywordString );
            } else {
                if (RawKeyword::useLine(line)) 
                    rawKeyword->addRawRecordString(line);
            }                    
            
            if (rawKeyword != NULL && rawKeyword->isFinished())
                return true;
        }
        
        return false;
    }



    boost::filesystem::path Parser::verifyValidInputPath(const std::string& inputPath) const {
        Logger::info("Verifying path: " + inputPath);
        boost::filesystem::path pathToInputFile(inputPath);
        if (!boost::filesystem::is_regular_file(pathToInputFile)) {
            Logger::error("Unable to open file with path: " + inputPath);
            throw std::invalid_argument("Given path is not a valid file-path, path: " + inputPath);
        }
        return pathToInputFile.parent_path();
    }

    void Parser::populateDefaultKeywords() {
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("GRIDUNIT", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("INCLUDE", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("RADFIN4", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("DIMENS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("START", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("GRIDOPTS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("ENDSCALE", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("EQLOPTS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("TABDIMS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("EQLDIMS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("REGDIMS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("FAULTDIM", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("WELLDIMS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("VFPPDIMS", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("RPTSCHED", 1)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("WHISTCTL", 1)));

        addKeyword(ParserKeywordConstPtr(new ParserKeyword("SUMMARY", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("TITLE", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("RUNSPEC", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("METRIC", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("SCHEDULE", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("SKIPREST", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("NOECHO", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("END", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("OIL", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("GAS", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("WATER", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("DISGAS", 0)));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("VAPOIL", 0)));
    }

} // namespace Opm
