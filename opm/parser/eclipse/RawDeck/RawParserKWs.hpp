/* 
 * File:   RawParserKW.h
 * Author: kflik
 *
 * Created on April 4, 2013, 12:12 PM
 */

#ifndef RAWPARSERKW_H
#define	RAWPARSERKW_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>
namespace Opm {

  class RawParserKWs {
  public:
    RawParserKWs();
    bool keywordExists(const std::string& keyword) const;
    unsigned int getFixedNumberOfRecords(const std::string& keyword) const;
    virtual ~RawParserKWs();
  private:
    std::map<std::string, unsigned int> m_keywordRecordLengths;
    void initializeFixedKeywordLenghts();
    void add(std::pair<std::string, unsigned int> keywordAndNumRecords);

  };
  typedef boost::shared_ptr<const RawParserKWs> RawParserKWsConstPtr;

}
#endif	/* RAWPARSERKW_H */

