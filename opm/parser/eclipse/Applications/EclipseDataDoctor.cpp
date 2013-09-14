/* 
 * File:   EclipseDataDoctor.cpp
 * Author: kflik
 *
 * Created on August 20, 2013, 1:19 PM
 */

#include <iostream>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>


void printDeckDiagnostics(Opm::DeckConstPtr deck, bool printAllKeywords) {
    int recognizedKeywords = 0;
    int unrecognizedKeywords = 0;
    for (size_t i = 0; i < deck->size(); i++) {
        if (!deck->getKeyword(i)->isKnown()) {
            unrecognizedKeywords++;
            std::cout << "Warning, this looks like a keyword, but is not in the configuration: " << deck->getKeyword(i)->name() << std::endl;
        } else
            recognizedKeywords++;

        if (printAllKeywords) {
            std::cout << "Keyword (" << i << "): " << deck->getKeyword(i)->name() << " " << std::endl;
        }
    }
    std::cout << "Number of recognized keywords:   " << recognizedKeywords << std::endl;
    std::cout << "Number of unrecognized keywords: " << unrecognizedKeywords << std::endl;
    std::cout << "Total number of keywords:        " << deck->size() << std::endl;

}
/*
 * 
 */
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <Filename>" << "[-l] (list keywords)" << std::endl;
        exit(1);
    }

    bool printKeywords = false;
    if (argc == 3) {
        std::string arg(argv[2]);
        if (arg == "-l")
            printKeywords = true;
    }

    Opm::ParserPtr parser(new Opm::Parser());
    std::string file = argv[1];
    Opm::DeckConstPtr deck = parser->parse(file, false);
    
    printDeckDiagnostics(deck, printKeywords);

    return 0;
}

