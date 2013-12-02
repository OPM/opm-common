/* 
 * File:   KeywordContainer.hpp
 * Author: kflik
 *
 * Created on June 3, 2013, 10:49 AM
 */

#ifndef KEYWORDCONTAINER_HPP
#define KEYWORDCONTAINER_HPP

#include <vector>
#include <map>

#include <memory>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>


namespace Opm {

    class KeywordContainer {
    public:
        KeywordContainer();
        bool hasKeyword(const std::string& keyword) const;
        size_t size() const;
        void addKeyword(DeckKeywordPtr keyword);
        DeckKeywordConstPtr getKeyword(const std::string& keyword, size_t index) const;
        DeckKeywordConstPtr getKeyword(const std::string& keyword) const;
        DeckKeywordConstPtr getKeyword(size_t index) const;

        const std::vector<DeckKeywordConstPtr>&  getKeywordList(const std::string& keyword) const;
        size_t numKeywords(const std::string& keyword) const;

    private:
        std::vector<DeckKeywordConstPtr> m_keywordList;
        std::map<std::string, std::vector<DeckKeywordConstPtr> > m_keywordMap;
    };
    typedef std::shared_ptr<KeywordContainer> KeywordContainerPtr;
    typedef std::shared_ptr<const KeywordContainer> KeywordContainerConstPtr;
}

#endif  /* KEYWORDCONTAINER_HPP */

