/* 
 * File:   EclipseDeck.cpp
 * Author: kflik
 * 
 * Created on March 11, 2013, 3:42 PM
 */

#include "EclipseDeck.hpp"

EclipseDeck::EclipseDeck() {
}

int EclipseDeck::getNumberOfKeywords() {
    return m_keywords.size();
}

const std::vector<std::string> EclipseDeck::getKeywords() {
    return m_keywords;
}

void EclipseDeck::addKeyword(const std::string &keyword) {
    m_keywords.push_back(keyword);
}

EclipseDeck::~EclipseDeck() {
}

