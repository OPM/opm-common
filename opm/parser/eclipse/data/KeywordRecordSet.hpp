/* 
 * File:   KeywordRawData.hpp
 * Author: kflik
 *
 * Created on March 20, 2013, 3:59 PM
 */

#ifndef KEYWORDRECORDSET_HPP
#define	KEYWORDRECORDSET_HPP

#include <string>
#include <utility>
#include <list>

namespace Opm {

    class KeywordRecordSet {
    public:
        KeywordRecordSet();
        KeywordRecordSet(const std::string& keyword, const std::list<std::string>& dataElements);
        KeywordRecordSet(const std::string& keyword);
        
        std::string getKeyword();
        void setKeyword(const std::string& keyword);
        void addDataElement(const std::string& element);
        virtual ~KeywordRecordSet();

    private:
        std::string m_keyword;
        std::list<std::string> m_data;
    };
}
#endif	/* KEYWORDRECORDSET_HPP */

