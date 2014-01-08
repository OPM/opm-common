/*
 * File:   KeywordDescriber.cpp
 * Author: atleh
 *
 * Created on December 19, 2013, 1:19 PM
 */

#include <iostream>
#include <utility>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserItem.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>


/*
 *
 */
void printKeywordInformation(Opm::ParserKeywordConstPtr keyword)
{
    std::string indent = "   ";

    std::cout << keyword->getName() << std::endl;
    std::cout << indent << "numItems:              " << keyword->numItems() << std::endl;
    std::cout << indent << "hasDimension:          " << keyword->hasDimension() << std::endl;
    std::cout << indent << "hasFixedSize:          " << keyword->hasFixedSize() << std::endl;
    if (keyword->hasFixedSize())
        std::cout << indent << "getFixedSize:          " << keyword->getFixedSize() << std::endl;
    std::cout << indent << "getSizeType:           " << keyword->getSizeType() << std::endl;
    std::pair<std::string, std::string> sizeDefinitionPair = keyword->getSizeDefinitionPair();
    std::cout << indent << "getSizeDefinitionPair: '" << sizeDefinitionPair.first << "', '" << sizeDefinitionPair.second << "'" << std::endl;

    Opm::ParserRecordPtr parserRecord = keyword->getRecord();
    std::vector<Opm::ParserItemConstPtr>::const_iterator iterator;
    for (iterator = parserRecord->begin(); iterator != parserRecord->end(); ++iterator) {
        std::cout << indent << (*iterator)->name() << std::endl;
        std::cout << indent << indent << "sizeType:           " << (*iterator)->sizeType() << std::endl;
        std::cout << indent << indent << "hasDimension:       " << (*iterator)->hasDimension() << std::endl;
        std::cout << indent << indent << "numDimensions:      " << (*iterator)->numDimensions() << std::endl;
        if ((*iterator)->numDimensions() == 1)
            std::cout << indent << indent << "getDimension(0):    '" << (*iterator)->getDimension((*iterator)->numDimensions()-1) << "'" << std::endl;
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
        keywords = parser->getKeywords();
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

        std::vector<std::string>::const_iterator iterator;
        for (iterator = keywords.begin(); iterator != keywords.end(); ++iterator) {
            Opm::ParserKeywordConstPtr keyword = parser->getKeyword(*iterator);
            printKeywordInformation(keyword);
        }

        return 0;
    }
