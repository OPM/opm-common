/* 
 * File:   DeckKeyword.hpp
 * Author: kflik
 *
 * Created on June 3, 2013, 12:55 PM
 */

#ifndef DECKKEYWORD_HPP
#define DECKKEYWORD_HPP

#include <string>
#include <vector>
#include <memory>

#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

namespace Opm {

    class DeckKeyword {
    public:
        DeckKeyword(const std::string& keywordName);
        DeckKeyword(const std::string& keywordName, bool knownKeyword);

        std::string name() const;
        size_t size() const;
        void addRecord(DeckRecordConstPtr record);
        DeckRecordConstPtr getRecord(size_t index) const;
        DeckRecordConstPtr getDataRecord() const;
        void setDataKeyword(bool isDataKeyword = true);
        bool isKnown() const;
        bool isDataKeyword() const;

        ssize_t getDeckIndex() const;
        void    setDeckIndex(size_t deckIndex);

        const std::vector<int>& getIntData() const;
        const std::vector<double>& getRawDoubleData() const;
        const std::vector<double>& getSIDoubleData() const;
        const std::vector<float>& getSIFloatData() const;
        const std::vector<float>& getRawFloatData() const;
        const std::vector<std::string>& getStringData() const;
        size_t getDataSize() const;

        std::vector<DeckRecordConstPtr>::const_iterator begin() const;
        std::vector<DeckRecordConstPtr>::const_iterator end() const;
    private:
        std::string m_keywordName;
        std::vector<DeckRecordConstPtr> m_recordList;
        bool m_knownKeyword;
        ssize_t m_deckIndex;
        bool m_isDataKeyword;
    };
    typedef std::shared_ptr<DeckKeyword> DeckKeywordPtr;
    typedef std::shared_ptr<const DeckKeyword> DeckKeywordConstPtr;
}

#endif  /* DECKKEYWORD_HPP */

