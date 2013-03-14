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
    int GetNumberOfKeywords();
    const std::vector<std::string> GetKeywords();
    void AddKeyword(const std::string &keyword);
    virtual ~EclipseDeck();
private:
    int numberOfKeywords;
    std::vector<std::string> keywords; 
};

#endif	/* ECLIPSEDECK_H */

