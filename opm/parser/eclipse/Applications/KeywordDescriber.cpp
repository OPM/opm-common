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
#include <utility>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>


void printKeywordInformation(Opm::ParserPtr parser, std::vector<std::string> keywords)
{
    std::string indent = " ";
    std::string keywordSeparator = "---------\n";
    std::string itemSeparator = "\n";

    std::vector<std::string>::const_iterator iterator;
    for (iterator = keywords.begin(); iterator != keywords.end(); ++iterator) {
        Opm::ParserKeywordConstPtr keyword = parser->getKeyword(*iterator);

        std::cout << keyword->getName() << std::endl;
        std::cout << indent << "Number of items: " << keyword->numItems() << std::endl;
        std::cout << indent << "Has dimension information: " << keyword->hasDimension() << std::endl;
        switch (keyword->getSizeType()) {
            case Opm::ParserKeywordSizeEnum::SLASH_TERMINATED: {
                std::cout << indent << "Size type: SLASH_TERMINATED" << std::endl;
                break;
            }
            case Opm::ParserKeywordSizeEnum::FIXED: {
                std::cout << indent << "Size type: FIXED" << std::endl;
                if (keyword->hasFixedSize())
                    std::cout << indent << "Fixed size: " << keyword->getFixedSize() << std::endl;
                break;
            }
            case Opm::ParserKeywordSizeEnum::OTHER_KEYWORD_IN_DECK: {
                std::cout << indent << "Size type: OTHER" << std::endl;
                std::pair<std::string, std::string> sizeDefinitionPair = keyword->getSizeDefinitionPair();
                std::cout << indent << "Size defined by: " << sizeDefinitionPair.first << ", " << sizeDefinitionPair.second << std::endl;
                break;
            }
            default:{
                std::cout << indent << "Size type: UNKNOWN" << std::endl;
                break;
            }
        }


        std::cout << itemSeparator;
        std::cout << indent << "List of items:" << std::endl;
        Opm::ParserRecordPtr parserRecord = keyword->getRecord();
        std::vector<Opm::ParserItemConstPtr>::const_iterator iterator;
        for (iterator = parserRecord->begin(); iterator != parserRecord->end(); ++iterator) {
            std::cout << indent << (*iterator)->name() << std::endl;
            switch ((*iterator)->sizeType()) {
                case Opm::ParserItemSizeEnum::ALL: {
                    std::cout << indent << indent << "SizeType: ALL" << std::endl;
                    break;
                }
                case Opm::ParserItemSizeEnum::SINGLE: {
                    std::cout << indent << indent << "SizeType: SINGLE" << std::endl;
                    break;
                }
            }

            std::cout << indent << indent << "Has dimension information: " << (*iterator)->hasDimension() << std::endl;
            if ((*iterator)->numDimensions() == 1)
                std::cout << indent << indent << "Dimension: " << (*iterator)->getDimension((*iterator)->numDimensions()-1) << std::endl;
            std::cout << itemSeparator;
        }
        std::cout << keywordSeparator;
    }
}

void printKeywordInformationOBSOLETE(Opm::ParserKeywordConstPtr keyword)
{
    std::string indent = " ";
    std::string keywordSeparator = "---------\n";
    std::string itemSeparator = "\n";

    std::cout << keyword->getName() << std::endl;
    std::cout << indent << "Number of items: " << keyword->numItems() << std::endl;
    std::cout << indent << "Has dimension information: " << keyword->hasDimension() << std::endl;
    switch (keyword->getSizeType()) {
        case Opm::ParserKeywordSizeEnum::SLASH_TERMINATED: {
            std::cout << indent << "Size type: SLASH_TERMINATED" << std::endl;
            break;
        }
        case Opm::ParserKeywordSizeEnum::FIXED: {
            std::cout << indent << "Size type: FIXED" << std::endl;
            if (keyword->hasFixedSize())
                std::cout << indent << "Fixed size: " << keyword->getFixedSize() << std::endl;
            break;
        }
        case Opm::ParserKeywordSizeEnum::OTHER_KEYWORD_IN_DECK: {
            std::cout << indent << "Size type: OTHER" << std::endl;
            std::pair<std::string, std::string> sizeDefinitionPair = keyword->getSizeDefinitionPair();
            std::cout << indent << "Size defined by: " << sizeDefinitionPair.first << ", " << sizeDefinitionPair.second << std::endl;
            break;
        }
        default:{
            std::cout << indent << "Size type: UNKNOWN" << std::endl;
            break;
        }
    }


    std::cout << itemSeparator;
    std::cout << indent << "List of items:" << std::endl;
    Opm::ParserRecordPtr parserRecord = keyword->getRecord();
    std::vector<Opm::ParserItemConstPtr>::const_iterator iterator;
    for (iterator = parserRecord->begin(); iterator != parserRecord->end(); ++iterator) {
        std::cout << indent << (*iterator)->name() << std::endl;
        switch ((*iterator)->sizeType()) {
            case Opm::ParserItemSizeEnum::ALL: {
                std::cout << indent << indent << "SizeType: ALL" << std::endl;
                break;
            }
            case Opm::ParserItemSizeEnum::SINGLE: {
                std::cout << indent << indent << "SizeType: SINGLE" << std::endl;
                break;
            }
        }

        std::cout << indent << indent << "Has dimension information: " << (*iterator)->hasDimension() << std::endl;
        if ((*iterator)->numDimensions() == 1)
            std::cout << indent << indent << "Dimension: " << (*iterator)->getDimension((*iterator)->numDimensions()-1) << std::endl;
        std::cout << itemSeparator;
    }
    std::cout << keywordSeparator;
}

bool parseCommandLineForAllKeywordsOption(char** argv)
{
    bool allKeywords = false;
    std::string arg(argv[1]);
    if (arg == "-a")
        allKeywords = true;

    return allKeywords;
}

std::vector<std::string> createListOfKeywordsToDescribe(char** argv, bool allKeywords, Opm::ParserPtr parser)
{
    std::vector<std::string> keywords;
    if (allKeywords) {
        keywords = parser->getAllKeywords();
    } else {
        std::string keywordName = argv[1];
        keywords.push_back(keywordName);
    }

    return keywords;
}

int main(int argc, char** argv) {
        if (argc < 2) {
            std::cout << "Usage: " << argv[0] << " <Keywordname>|-a (all keywords)" << std::endl;
            exit(1);
        }

        bool allKeywords = parseCommandLineForAllKeywordsOption(argv);

        Opm::ParserPtr parser(new Opm::Parser());

        std::vector<std::string> keywords = createListOfKeywordsToDescribe(argv, allKeywords, parser);

        printKeywordInformation(parser, keywords);

        return 0;
    }
