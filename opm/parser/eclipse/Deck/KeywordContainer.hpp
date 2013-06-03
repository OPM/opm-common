/* 
 * File:   KeywordContainer.hpp
 * Author: kflik
 *
 * Created on June 3, 2013, 10:49 AM
 */

#ifndef KEYWORDCONTAINER_HPP
#define	KEYWORDCONTAINER_HPP

#include <vector>
#include <map>

#include <boost/shared_ptr.hpp>
#include <opm/parser/eclipse/Deck/DeckKW.hpp>


namespace Opm {

    class KeywordContainer {
    public:
        KeywordContainer();
        bool hasKeyword(const std::string& keyword) const;
        size_t size() const;
        void addKeyword(DeckKWConstPtr keyword);

    private:
        std::vector<DeckKWConstPtr> m_keywordList;
        std::map<std::string, std::vector<DeckKWConstPtr> > m_keywordMap;
    };
    typedef boost::shared_ptr<KeywordContainer> KeywordContainerPtr;
    typedef boost::shared_ptr<const KeywordContainer> KeywordContainerConstPtr;
}

#endif	/* KEYWORDCONTAINER_HPP */

