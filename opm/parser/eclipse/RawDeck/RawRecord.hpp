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
        static const std::string SEPARATORS;

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
        void splitSingleRecordString();
        void processSeparatorCharacter(std::string& currentToken, const char& currentChar, char& tokenStarter);
        void processQuoteCharacters(std::string& currentToken, const char& currentChar, char& tokenStarter);
        void processNonSpecialCharacters(std::string& currentToken, const char& currentChar);
        bool charIsSeparator(char candidate);
        static unsigned int findTerminatingSlash(const std::string& singleRecordString);
    };
    typedef boost::shared_ptr<RawRecord> RawRecordPtr;
}

#endif	/* RECORD_HPP */

