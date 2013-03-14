/* 
 * File:   Parser.h
 * Author: kflik
 *
 * Created on March 11, 2013, 3:40 PM
 */
#ifndef PARSER_H
#define	PARSER_H

#include "EclipseDeck.hpp"
#include <string>

class Parser {
public:
    Parser();
    Parser(std::string path);
    EclipseDeck Parse();
    virtual ~Parser();
private:
    //EclipseDeck deck;
    std::string pathToEclipseFile;
};

#endif	/* PARSER_H */

