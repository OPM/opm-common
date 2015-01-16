/*
  Copyright 2014 Andreas Lauser

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef OPM_COUNTERLOG_HPP
#define OPM_COUNTERLOG_HPP

#include <string>
#include <vector>
#include <tuple>
#include <memory>
#include <map>

#include <opm/parser/eclipse/OpmLog/LogBackend.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>

namespace Opm {
/*!
 * \brief Provides a simple sytem for log message which are found by the
 *        Parser/Deck/EclipseState classes during processing the deck.
 */
    class CounterLog : public LogBackend {
public:

    CounterLog(int64_t messageMask);
    CounterLog();

    size_t numErrors() const;
    size_t numWarnings() const;
    size_t numNotes() const;
    size_t numMessages(int64_t messageType) const;


    void addMessage(int64_t messageFlag ,
                    const std::string& message);


    const std::string& getFileName(size_t msgIdx) const;
    int getLineNumber(size_t msgIdx) const;
    int64_t getMessageType(size_t msgIdx) const;
    const std::string& getDescription(size_t msgIdx) const;

    void clear();
    /*!
     * \brief This method takes the information provided by the methods above and returns
     *        them in a fully-formatted string.
     *
     * It is thus covenience method to convert a log message into a GCC-like format,
     * e.g. a "Note" message triggered by the file "SPE1DECK.DATA" on line 15 which says
     * that no grid can be constructed would yield:
     *
     * SPE1DECK.DATA:15:note: No grid found.
     */
    const std::string getFormattedMessage(size_t msgIdx) const;
    ~CounterLog() {};
private:
    typedef std::tuple<std::string,
                       int,
                       int64_t,
                       std::string> MessageTuple;

    std::vector<MessageTuple> m_messages;
    std::map<int64_t , size_t> m_count;
};

typedef std::shared_ptr<CounterLog> CounterLogPtr;
typedef std::shared_ptr<const CounterLog> CounterLogConstPtr;
} // namespace Opm

#endif

