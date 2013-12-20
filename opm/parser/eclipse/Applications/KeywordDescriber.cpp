/*
 * File:   KeywordDescriber.cpp
 * Author: atleh
 *
 * Created on December 19, 2013, 1:19 PM
 */

#include <iostream>
#include <utility>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>


/*
 *
 */
void printKeywordInformation(Opm::ParserKeywordConstPtr keyword)
{
    std::cout << "getName: " << keyword->getName() << std::endl;
//        ParserRecordPtr recordPtr = keyword->getRecord();

    std::cout << "numItems: " << keyword->numItems() << std::endl;
    std::cout << "hasDimension: " << keyword->hasDimension() << std::endl;
    std::cout << "hasFixedSize: " << keyword->hasFixedSize() << std::endl;
    if (keyword->hasFixedSize())
        std::cout << "getFixedSize: " << keyword->getFixedSize() << std::endl;
    std::cout << "isTableCollection: " << keyword->isTableCollection() << std::endl;
    std::cout << "getSizeType: " << keyword->getSizeType() << std::endl;
    std::pair<std::string, std::string> sizeDefinitionPair = keyword->getSizeDefinitionPair();
    std::cout << "getSizeDefinitionPair: '" << sizeDefinitionPair.first << "', '" << sizeDefinitionPair.second << "'" << std::endl;
    std::cout << "isDataKeyword: " << keyword->isDataKeyword() << std::endl;
}

int main(int argc, char** argv) {
        if (argc < 2) {
            std::cout << "Usage: " << argv[0] << " <Keywordname>|-a (all keywords)" << std::endl;
            exit(1);
        }

        bool allKeywords = false;
        std::string arg(argv[1]);
        if (arg == "-a")
            allKeywords = true;

        Opm::ParserPtr parser(new Opm::Parser());
        std::string keywordName = argv[1];

        //parser->listKeywords();
        std::list<std::string> keywords;
        if (allKeywords) {
            parser->getKeywords(&keywords);
//            std::list<std::string>::const_iterator iterator;
//            for (iterator = keywords.begin(); iterator != keywords.end(); ++iterator) {
//                std::cout << *iterator;
//            }
        } else {
            keywords.push_back(keywordName);
        }

        std::list<std::string>::const_iterator iterator;
        for (iterator = keywords.begin(); iterator != keywords.end(); ++iterator) {
            Opm::ParserKeywordConstPtr keyword = parser->getKeyword(*iterator);
            printKeywordInformation(keyword);
        }


        return 0;
    }
