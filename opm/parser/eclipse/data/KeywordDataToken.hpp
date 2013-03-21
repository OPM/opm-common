/* 
 * File:   KeywordRawData.hpp
 * Author: kflik
 *
 * Created on March 20, 2013, 3:59 PM
 */

#ifndef KEYWORDATATOKEN_HPP
#define	KEYWORDATATOKEN_HPP

#include <string>
#include <utility>
#include <list>

namespace Opm {

    class KeywordDataToken {
    public:
        KeywordDataToken();
        KeywordDataToken(const std::string& keyword, const std::list<std::string>& dataElements);
        KeywordDataToken(const std::string& keyword);
        
        std::string getKeyword();
        void setKeyword(const std::string& keyword);
        void addDataElement(const std::string& element);
        virtual ~KeywordDataToken();

    private:
        std::string m_keyword;
        std::list<std::string> m_data;
    };
}
#endif	/* KEYWORDATATOKEN_HPP */

