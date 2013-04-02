/* 
 * File:   Record.hpp
 * Author: kflik
 *
 * Created on March 21, 2013, 10:44 AM
 */

#ifndef RECORD_HPP
#define	RECORD_HPP

#include <string>
#include <vector>


namespace Opm {

    class RawRecord {
    public:
        static const char SLASH;
        static const char QUOTE;
        RawRecord();
        RawRecord(const std::string& singleRecordString);
        void getRecordString(std::string& recordString);
        void getRecords(std::vector<std::string>& recordItems);
        static bool isCompleteRecordString(const std::string& candidateRecordString);
        virtual ~RawRecord();
    private:
        std::string m_sanitizedRecordString;
        std::vector<std::string> m_recordItems;
        void setRecordString(const std::string& singleRecordString);
        bool stringContains(std::string collection, char candidate);
        static unsigned int findTerminatingSlash(const std::string& singleRecordString);
    };
    typedef boost::shared_ptr<RawRecord> RawRecordPtr;
}

#endif	/* RECORD_HPP */

