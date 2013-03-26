/* 
 * File:   Record.hpp
 * Author: kflik
 *
 * Created on March 21, 2013, 10:44 AM
 */

#ifndef RECORD_HPP
#define	RECORD_HPP

#include <string>
#include <list>
#include <vector>


namespace Opm {

    class RawRecord {
    public:
        static const char SLASH;
        static const char QUOTE;
        RawRecord();
        RawRecord(const std::string& singleRecordString);
        void getRecord(std::string& recordString);
        static bool isCompleteRecordString(const std::string& candidateRecordString);
        virtual ~RawRecord();
    private:
        std::string m_sanitizedRecordString;
        void sanitizeInputString(const std::string& singleRecordString);
        static unsigned int findTerminatingSlash(const std::string& singleRecordString);
    };
    typedef boost::shared_ptr<RawRecord> RawRecordPtr;
}

#endif	/* RECORD_HPP */

