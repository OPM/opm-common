/* 
 * File:   EclipseDeck.cpp
 * Author: kflik
 * 
 * Created on March 11, 2013, 3:42 PM
 */

#include "EclipseDeck.hpp"

EclipseDeck::EclipseDeck() {
}

int EclipseDeck::GetNumberOfKeywords() {
    return keywords.size();
}

std::vector<std::string> EclipseDeck::GetKeywords() {
    return keywords;
}

EclipseDeck::~EclipseDeck() {
}

