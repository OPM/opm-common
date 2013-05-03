/* 
 * File:   RawDeck.hpp
 * Author: kflik
 *
 * Created on March 21, 2013, 2:02 PM
 */

#ifndef RAWDECK_HPP
#define RAWDECK_HPP
#include <list>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include "opm/parser/eclipse/Logger/Logger.hpp"
#include "RawKeyword.hpp"

namespace Opm {

    class RawDeck {
    public:
        RawDeck();
        void readDataIntoDeck(const std::string& path);
        RawKeywordPtr getKeyword(const std::string& keyword);
        unsigned int getNumberOfKeywords();
        virtual ~RawDeck();
    private:
        std::list<RawKeywordPtr> m_keywords;
        void addRawRecordStringToRawKeyword(const std::string& line, RawKeywordPtr currentRawKeyword);
        bool looksLikeData(const std::string& line);
        void checkInputFile(const std::string& inputPath);
    };
  
    typedef boost::shared_ptr<RawDeck> RawDeckPtr;
}

#endif  /* RAWDECK_HPP */

