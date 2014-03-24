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
        DeckKeywordPtr getKeyword(const std::string& keyword, size_t index) const;
        DeckKeywordPtr getKeyword(const std::string& keyword) const;
        DeckKeywordPtr      getKeyword(size_t index) const;

        const std::vector<DeckKeywordPtr>&  getKeywordList(const std::string& keyword) const;
        size_t numKeywords(const std::string& keyword) const;

        std::vector<DeckKeywordPtr>::iterator begin();
        std::vector<DeckKeywordPtr>::iterator end();

    private:
        std::vector<DeckKeywordPtr> m_keywordList;
        std::map<std::string, std::vector<DeckKeywordPtr> > m_keywordMap;
    };
    typedef std::shared_ptr<KeywordContainer> KeywordContainerPtr;
    typedef std::shared_ptr<const KeywordContainer> KeywordContainerConstPtr;
}

#endif  /* KEYWORDCONTAINER_HPP */

