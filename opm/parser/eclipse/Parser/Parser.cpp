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
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/RawDeck/RawConsts.hpp>
#include <opm/parser/eclipse/Logger/Logger.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>


namespace Opm {

    Parser::Parser() {
        populateDefaultKeywords();
    }

    DeckPtr Parser::parse(const std::string &path) {
        RawDeckPtr rawDeck = readToRawDeck(path);
        return parseFromRawDeck(rawDeck);
    }

    DeckPtr Parser::parseFromRawDeck(RawDeckConstPtr rawDeck) {
        DeckPtr deck(new Deck());
        for (size_t i = 0; i < rawDeck->size(); i++) {
            RawKeywordConstPtr rawKeyword = rawDeck->getKeyword(i);

            if (hasKeyword(rawKeyword->getKeywordName())) {
                ParserKeywordConstPtr parserKeyword = m_parserKeywords[rawKeyword->getKeywordName()];
                DeckKeywordConstPtr deckKeyword = parserKeyword->parse(rawKeyword);
                deck->addKeyword(deckKeyword);
            } else
                std::cerr << "Keyword: " << rawKeyword->getKeywordName() << " is not recognized, skipping this." << std::endl;
        }
        return deck;
    }

    void Parser::addKeyword(ParserKeywordConstPtr parserKeyword) {
        m_parserKeywords.insert(std::make_pair(parserKeyword->getName(), parserKeyword));
    }

    bool Parser::hasKeyword(const std::string& keyword) const {
        return m_parserKeywords.find(keyword) != m_parserKeywords.end();
    }

    RawDeckPtr Parser::readToRawDeck(const std::string& path) const {
        RawDeckPtr rawDeck(new RawDeck());
        readToRawDeck(rawDeck, path);
        return rawDeck;
    }

    /// The main data reading function, reads one and one keyword into the RawDeck
    /// If the INCLUDE keyword is found, the specified include file is inline read into the RawDeck.
    /// The data is read into a keyword, record by record, until the fixed number of records specified
    /// in the RawParserKeyword is met, or till a slash on a separate line is found.

    void Parser::readToRawDeck(RawDeckPtr rawDeck, const std::string& path) const {
        boost::filesystem::path dataFolderPath = verifyValidInputPath(path);
        {
            std::ifstream inputstream;
            inputstream.open(path.c_str());

            std::string line;
            RawKeywordPtr currentRawKeyword;
            while (std::getline(inputstream, line)) {
                std::string keywordString;
                if (currentRawKeyword == NULL) {
                    if (RawKeyword::tryParseKeyword(line, keywordString)) {
                        currentRawKeyword = RawKeywordPtr(new RawKeyword(keywordString));
                        if (isFixedLenghtKeywordFinished(currentRawKeyword)) {
                            rawDeck->addKeyword(currentRawKeyword);
                            currentRawKeyword.reset();
                        }
                    }
                } else if (currentRawKeyword != NULL && RawKeyword::lineContainsData(line)) {
                    currentRawKeyword->addRawRecordString(line);
                    if (isFixedLenghtKeywordFinished(currentRawKeyword)) {
                        // The INCLUDE keyword has fixed lenght 1, will hit here
                        if (currentRawKeyword->getKeywordName() == Opm::RawConsts::include)
                            processIncludeKeyword(rawDeck, currentRawKeyword, dataFolderPath);
                        else
                            rawDeck->addKeyword(currentRawKeyword);

                        currentRawKeyword.reset();
                    }
                } else if (currentRawKeyword != NULL && RawKeyword::lineTerminatesKeyword(line)) {
                    if (!currentRawKeyword->isPartialRecordStringEmpty()) {
                        // This is an error in the input file, but sometimes occurs
                        currentRawKeyword->addRawRecordString(std::string(1, Opm::RawConsts::slash));
                    }
                    // Don't need to check for include here, since only non-fixed lenght keywords come here.
                    rawDeck->addKeyword(currentRawKeyword);
                    currentRawKeyword.reset();
                }
            }
            inputstream.close();
        }
    }

    bool Parser::isFixedLenghtKeywordFinished(RawKeywordConstPtr rawKeyword) const {
        bool fixedSizeReached = false;
        if (hasKeyword(rawKeyword->getKeywordName())) {
            ParserKeywordConstPtr parserKeyword = m_parserKeywords.find(rawKeyword->getKeywordName())->second;
            if (parserKeyword->hasFixedSize())
                fixedSizeReached = rawKeyword->size() == parserKeyword->getFixedSize();
        }

        return fixedSizeReached;
    }

    void Parser::processIncludeKeyword(RawDeckPtr rawDeck, RawKeywordConstPtr keyword, const boost::filesystem::path& dataFolderPath) const {
        RawRecordConstPtr firstRecord = keyword->getRecord(0);
        std::string includeFileString = firstRecord->getItem(0);
        boost::filesystem::path pathToIncludedFile(dataFolderPath);
        pathToIncludedFile /= includeFileString;

        readToRawDeck(rawDeck, pathToIncludedFile.string());
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
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("GRIDUNIT", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("INCLUDE", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("RADFIN4", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("DIMENS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("START", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("GRIDOPTS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("ENDSCALE", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("EQLOPTS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("TABDIMS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("EQLDIMS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("REGDIMS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("FAULTDIM", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("WELLDIMS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("VFPPDIMS", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("RPTSCHED", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("WHISTCTL", ParserKeywordSizeConstPtr(new ParserKeywordSize(1)))));

        addKeyword(ParserKeywordConstPtr(new ParserKeyword("SUMMARY", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("TITLE", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("RUNSPEC", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("METRIC", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("SCHEDULE", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("SKIPREST", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("NOECHO", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("END", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("OIL", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("GAS", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("WATER", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("DISGAS", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
        addKeyword(ParserKeywordConstPtr(new ParserKeyword("VAPOIL", ParserKeywordSizeConstPtr(new ParserKeywordSize(0)))));
    }

} // namespace Opm
