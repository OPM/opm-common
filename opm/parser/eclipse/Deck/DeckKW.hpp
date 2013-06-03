/* 
 * File:   DeckKW.hpp
 * Author: kflik
 *
 * Created on June 3, 2013, 12:55 PM
 */

#ifndef DECKKW_HPP
#define	DECKKW_HPP

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

namespace Opm {

    class DeckKW {
    public:
        DeckKW(const std::string& keywordName);
        std::string name() const;
        size_t size() const;
        void addRecord(DeckRecordConstPtr record);
        DeckRecordConstPtr getRecord(size_t index) const;
        
    private:
        std::string m_keywordName;
        std::vector<DeckRecordConstPtr> m_recordList;

    };
    typedef boost::shared_ptr<DeckKW> DeckKWPtr;
    typedef boost::shared_ptr<const DeckKW> DeckKWConstPtr;
}

#endif	/* DECKKW_HPP */

