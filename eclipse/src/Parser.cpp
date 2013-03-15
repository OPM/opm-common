/* 
 * File:   Parser.cpp
 * Author: kflik
 * 
 * Created on March 11, 2013, 3:40 PM
 */
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
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
        if (line.substr(0, 2) == "--") {
            m_log.append("COMMENT LINE   < " + line + ">\n");
        }
        else if (boost::algorithm::trim_copy(line).length() == 0) {
            m_log.append("EMPTY LINE     <" + line + ">\n");
        }
        else if (line.substr(0, 1) != " " && boost::algorithm::to_upper_copy(line) == line) {
            deck.addKeyword(line);
            m_log.append("KEYWORD LINE   <" + line + ">\n");
        }
        else {
            m_log.append("SOMETHING ELSE <" + line + ">\n");
        }
    }
    writeLogToFile();
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

