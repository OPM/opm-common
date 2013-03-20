/* 
 * File:   KeywordRawData.hpp
 * Author: kflik
 *
 * Created on March 20, 2013, 3:59 PM
 */

#ifndef KEYWORDRAWDATA_HPP
#define	KEYWORDRAWDATA_HPP

#include <string>
#include <utility>
#include <list>

namespace Opm {

    class KeywordRawData {
    public:
        KeywordRawData();
        void addKeywordDataBlob(const std::string& keyword, const std::list<std::string>& blob);
        int numberOfKeywords();
        virtual ~KeywordRawData();
    private:
        std::list< std::pair< std::string, std::list<std::string > > > m_keywordRawData;
    };
}
#endif	/* KEYWORDRAWDATA_HPP */

