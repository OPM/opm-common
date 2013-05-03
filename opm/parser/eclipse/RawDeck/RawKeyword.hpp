/* 
 * File:   KeywordRawData.hpp
 * Author: kflik
 *
 * Created on March 20, 2013, 3:59 PM
 */

#ifndef RAWKEYWORD_HPP
#define RAWKEYWORD_HPP

#include <string>
#include <utility>
#include <list>
#include <boost/shared_ptr.hpp>

#include "opm/parser/eclipse/Logger/Logger.hpp"
#include "RawRecord.hpp"

namespace Opm {
    class RawKeyword {
    public:
        RawKeyword();
        RawKeyword(const std::string& keyword);
        static bool tryParseKeyword(const std::string& line, std::string& result);
        std::string getKeyword();
        void getRecords(std::list<RawRecordPtr>& records);
        unsigned int getNumberOfRecords();
        void setKeyword(const std::string& keyword);
        void addRawRecordString(const std::string& partialRecordString);
        virtual ~RawKeyword();

    private:
        std::string m_keyword;
        std::list<RawRecordPtr> m_records;
        std::string m_partialRecordString;
        static bool isValidKeyword(const std::string& keywordCandidate);
    };
    typedef boost::shared_ptr<RawKeyword> RawKeywordPtr;
}
#endif  /* RAWKEYWORD_HPP */

