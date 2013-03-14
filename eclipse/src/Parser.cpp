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
    m_dataFilePath = path;
}

EclipseDeck Parser::parse() {
    EclipseDeck deck;
    m_log.append("Initializing inputstream from file: " + m_dataFilePath + "\n");

    ifstream inputstream;
    inputstream.open(m_dataFilePath.c_str());

    if (!inputstream.is_open()) {
        m_log.append("ERROR: unable to open file");
        return deck;
    }
    
    std::string line;
    while (!inputstream.eof()) {
        std::getline(inputstream, line);
        if (line.substr(0, 2) != "--") {
            deck.addKeyword(line);
        }
        m_log.append("Line: " + line + "\n");
    }

    inputstream.close();
    return deck;
}

void Parser::writeLogToFile() {
    std::ofstream logfile;
    logfile.open("log.txt");
    logfile << m_log;
    logfile.close();
}

Parser::~Parser() {
}

