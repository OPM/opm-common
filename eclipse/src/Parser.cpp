/* 
 * File:   Parser.cpp
 * Author: kflik
 * 
 * Created on March 11, 2013, 3:40 PM
 */
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
using std::ifstream;

#include "Parser.hpp"

Parser::Parser() {
}

Parser::Parser(const std::string &path) {
    dataFilePath = path;
}

EclipseDeck Parser::Parse() {
    EclipseDeck deck;

    log.append("Initializing inputstream from file: " + dataFilePath + "\n");

    ifstream inputstream;
    inputstream.open(dataFilePath.c_str());

    if (!inputstream.is_open()) {
        log.append("ERROR: unable to open file");
        return deck;
    }
    std::string line;

    while (!inputstream.eof()) {
        std::getline(inputstream, line);
        if (line.substr(0, 2) != "--") {
            deck.AddKeyword(line);
        }
        log.append("Linje: " + line + "\n");
    }

    inputstream.close();

    std::ofstream logfile;
    logfile.open("log.txt");
    logfile << log;
    logfile.close();

    return deck;
}

Parser::~Parser() {
}

