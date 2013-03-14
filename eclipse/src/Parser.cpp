/* 
 * File:   Parser.cpp
 * Author: kflik
 * 
 * Created on March 11, 2013, 3:40 PM
 */
#include <boost/filesystem.hpp>

#include <fstream>
using std::ifstream;

#include "Parser.hpp"

Parser::Parser() {
}

Parser::Parser(const std::string &path) {
    boost::filesystem::path currentPath;
    currentPath = boost::filesystem::current_path();
    
    // Read keyword definition from file
}

EclipseDeck Parser::Parse() {
    
    EclipseDeck deck;
    return deck;
}

Parser::~Parser() {
}

