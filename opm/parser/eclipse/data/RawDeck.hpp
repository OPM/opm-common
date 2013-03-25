/* 
 * File:   RawDeck.hpp
 * Author: kflik
 *
 * Created on March 21, 2013, 2:02 PM
 */

#ifndef RAWDECK_HPP
#define	RAWDECK_HPP
#include <list>
#include <iostream>
#include "opm/parser/eclipse/Logger.hpp"
#include "RawKeyword.hpp"

namespace Opm {

    class RawDeck {
    public:
        RawDeck();
        void readDataIntoDeck(const std::string& path);
        RawKeyword* getKeyword(const std::string& keyword);
        unsigned int getNumberOfKeywords();
        virtual ~RawDeck();
    private:
        std::list<RawKeyword*> m_keywords;
        void addRawRecordStringToRawKeyword(const std::string& line, RawKeyword* currentRawKeyword);
        bool looksLikeData(const std::string& line);
        void checkInputFile(const std::string& inputPath);
    };
}

#endif	/* RAWDECK_HPP */

