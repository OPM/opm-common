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
#include <stdexcept>
#include <sstream>
#include <cassert>

#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>
#include <opm/parser/eclipse/OpmLog/CounterLog.hpp>



namespace Opm {
CounterLog::CounterLog() : LogBackend(Log::DefaultMessageTypes)
{
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    setOutStream(NULL);
}

CounterLog::CounterLog(std::ostream* os) : LogBackend(Log::DefaultMessageTypes)
{
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    setOutStream(os);
}

void CounterLog::setOutStream(std::ostream* os) {
    m_outStream = os;
}

size_t CounterLog::size() const {
    return m_messages.size();
}

size_t CounterLog::numErrors() const {
    return m_numErrors;
}

size_t CounterLog::numWarnings() const {
    return m_numWarnings;
}

size_t CounterLog::numNotes() const {
    return m_numNotes;
}

void CounterLog::addMessage(const std::string& fileName,
                                int lineNumber,
                                int64_t messageType,
                                const std::string& description) {

    if (includeMessage( messageType )) {
        switch (messageType) {
        case Log::MessageType::Note:
            ++m_numNotes;
            break;

        case Log::MessageType::Warning:
            ++m_numWarnings;
            break;

        case Log::MessageType::Error:
            ++m_numErrors;
            break;

        default:
            throw std::invalid_argument("Log messages must be of type Note, Warning or Error");
        }

        m_messages.push_back(MessageTuple(fileName, lineNumber, messageType, description));

        if (m_outStream) {
            (*m_outStream) << getFormattedMessage(size() - 1) << "\n";
            (*m_outStream) << (std::flush);
        }
    }
}

void CounterLog::addMessage(int64_t messageType , const std::string& message) {
    if (includeMessage( messageType ))
        addMessage("???" , -1 , messageType , message);
}




void CounterLog::addNote(const std::string& fileName,
                             int lineNumber,
                             const std::string& description) {
    addMessage(fileName, lineNumber, Log::MessageType::Note, description);
}

void CounterLog::addWarning(const std::string& fileName,
                                int lineNumber,
                                const std::string& description) {
    addMessage(fileName, lineNumber, Log::MessageType::Warning, description);
}

void CounterLog::addError(const std::string& fileName,
                              int lineNumber,
                              const std::string& description) {
    addMessage(fileName, lineNumber, Log::MessageType::Error, description);
}


void CounterLog::clear()
{
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    m_messages.clear();
}

void CounterLog::append(const CounterLog &other)
{
    for (size_t i = 0; i < other.size(); ++i) {
        addMessage(other.getFileName(i),
                   other.getLineNumber(i),
                   other.getMessageType(i),
                   other.getDescription(i));
    }
}

const std::string& CounterLog::getFileName(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<0>(m_messages[msgIdx]);
}

int CounterLog::getLineNumber(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<1>(m_messages[msgIdx]);
}

int64_t CounterLog::getMessageType(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<2>(m_messages[msgIdx]);
}

const std::string& CounterLog::getDescription(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<3>(m_messages[msgIdx]);
}

const std::string CounterLog::getFormattedMessage(size_t msgIdx) const {
    const std::string& description = getDescription( msgIdx );
    int64_t messageType = getMessageType( msgIdx );
    std::string prefixedMessage = Log::prefixMessage( messageType  , description);
    int lineNumber = getLineNumber(msgIdx);

    if (lineNumber > 0) {
        const std::string& filename = getFileName(msgIdx);
        return Log::fileMessage( filename , getLineNumber(msgIdx) , prefixedMessage);
    } else
        return prefixedMessage;
}



void CounterLog::printAll(std::ostream& os, size_t enabledTypes) const {
    for (size_t i = 0; i < size(); ++i)
        if (enabledTypes & getMessageType(i))
            os << getFormattedMessage(i) << "\n";
}


} // namespace Opm
