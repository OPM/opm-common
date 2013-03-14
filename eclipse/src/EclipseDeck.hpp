/* 
 * File:   EclipseDeck.h
 * Author: kflik
 *
 * Created on March 11, 2013, 3:42 PM
 */

#ifndef ECLIPSEDECK_H
#define	ECLIPSEDECK_H
#include <vector>
#include <string>

class EclipseDeck {
public:
    EclipseDeck();
    int getNumberOfKeywords();
    const std::vector<std::string> getKeywords();
    void addKeyword(const std::string &keyword);
    virtual ~EclipseDeck();
private:
    int m_numberOfKeywords;
    std::vector<std::string> m_keywords; 
};

#endif	/* ECLIPSEDECK_H */

