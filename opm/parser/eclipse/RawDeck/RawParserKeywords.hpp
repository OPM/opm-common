/* 
 * File:   RawParserKeyword.h
 * Author: kflik
 *
 * Created on April 4, 2013, 12:12 PM
 */

#ifndef RAWPARSERKEYWORD_H
#define	RAWPARSERKEYWORD_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
namespace Opm {

    /// Class holding information about the characteristics of all known fixed length keywords.
    /// The characteristics being held is the ones important for the raw parsing,
    /// these being the keywords name and fixed number of records for the keyword.

    class RawParserKeywords {
    public:
        RawParserKeywords();
        bool keywordExists(const std::string& keyword) const;
        unsigned int getFixedNumberOfRecords(const std::string& keyword) const;
        virtual ~RawParserKeywords();
    private:
        std::map<std::string, unsigned int> m_keywordRecordLengths;
        void initializeFixedKeywordLenghts();
        void add(std::pair<std::string, unsigned int> keywordAndNumRecords);

    };
    typedef boost::shared_ptr<const RawParserKeywords> RawParserKeywordsConstPtr;

}
#endif	/* RAWPARSERKEYWORD_H */

