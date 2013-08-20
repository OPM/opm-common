/* 
 * File:   EclipseDataDoctor.cpp
 * Author: kflik
 *
 * Created on August 20, 2013, 1:19 PM
 */

#include <iostream>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

/*
 * 
 */
int main(int argc, char** argv) {
    if (argc <= 1)
    {
        std::cout << "Usage: " << argv[0] << " <Filename>"  << "[-a] (list all keywords)" << std::endl;
        exit(1);
    }
    
    bool showKeywords = false;
    for (int i=1; i<argc; i++) {
        std::string arg(argv[i]);
        if (arg == "-a")
            showKeywords = true;
    }
    std::string file = argv[1];
    Opm::ParserPtr parser(new Opm::Parser(JSON_CONFIG_FILE));
    try {
        Opm::DeckConstPtr deck = parser->parse(file);
        std::cout << "Number of keywords: " << deck->size() << std::endl;
        if (showKeywords) {
            for (size_t i=0; i < deck->size(); i++) {
                std::cout << "Keyword" << ": " << deck->getKeyword(i)->name() << std::endl;
            }
        }
    }
    catch (std::invalid_argument exception) {
        std::cout << "Unable to read file, error:" << std::endl << exception.what() << std::endl;
    }
    return 0;
}

