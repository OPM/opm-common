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


void printKeyword(Opm::ParserKeywordConstPtr keyword)
{
    std::string indent = " ";
    std::cout << keyword->getName() << std::endl;
    if (keyword->getHelpText().length() > 0) {
        std::cout << indent << "Help text: " << keyword->getHelpText() << std::endl;
    }
    std::cout << indent << "Number of items: " << keyword->numItems() << std::endl;
    std::cout << indent << "Has dimension information: " << keyword->hasDimension() << std::endl;
    std::cout << indent << "Size type: " << ParserKeywordSizeEnum2String(keyword->getSizeType()) << std::endl;
    switch (keyword->getSizeType()) {
        case Opm::ParserKeywordSizeEnum::FIXED: {
            if (keyword->hasFixedSize())
                std::cout << indent << "Fixed size: " << keyword->getFixedSize() << std::endl;
            break;
        }
        case Opm::ParserKeywordSizeEnum::OTHER_KEYWORD_IN_DECK: {
            std::pair<std::string, std::string> sizeDefinitionPair = keyword->getSizeDefinitionPair();
            std::cout << indent << "Size defined by: " << sizeDefinitionPair.first << ", " << sizeDefinitionPair.second << std::endl;
            break;
        }
        default:{
            break;
        }
    }
}

void printItem(Opm::ParserItemConstPtr item, std::string indent)
{
    std::cout << indent << item->name() << std::endl;
    if (item->getHelpText().length() > 0) {
        std::cout << indent << "Help text: " << item->getHelpText() << std::endl;
    }
    std::cout << indent << "SizeType: " << ParserItemSizeEnum2String(item->sizeType()) << std::endl;
    std::cout << indent << "Has dimension information: " << item->hasDimension() << std::endl;
    if (item->numDimensions() == 1)
        std::cout << indent << "Dimension: " << item->getDimension(item->numDimensions()-1) << std::endl;
    std::cout << std::endl;
}

void printItems(Opm::ParserKeywordConstPtr keyword)
{
    std::string indent = "  ";
    std::cout << std::endl;
    std::cout << indent << "List of items:" << std::endl;
    Opm::ParserRecordPtr parserRecord = keyword->getRecord();
    std::vector<Opm::ParserItemConstPtr>::const_iterator iterator;
    for (iterator = parserRecord->begin(); iterator != parserRecord->end(); ++iterator) {
        Opm::ParserItemConstPtr item = *iterator;
        printItem(item, indent);
    }
}

void printKeywords (Opm::ParserPtr parser, std::vector<std::string> keywords)
{
    std::vector<std::string>::const_iterator iterator;
    for (iterator = keywords.begin(); iterator != keywords.end(); ++iterator) {
        Opm::ParserKeywordConstPtr keyword = parser->getKeyword(*iterator);
        printKeyword(keyword);
        printItems(keyword);
    }
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
    printKeywords(parser, keywords);

    return 0;
}
