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
#include <opm/parser/eclipse/OpmLog/MessageCounter.hpp>



namespace Opm {
MessageCounter::MessageCounter() : LogBackend(Log::AllMessageTypes)
{
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    setOutStream(NULL);
}

MessageCounter::MessageCounter(std::ostream* os) : LogBackend(Log::AllMessageTypes)
{
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    setOutStream(os);
}

void MessageCounter::setOutStream(std::ostream* os) {
    m_outStream = os;
}

size_t MessageCounter::size() const {
    return m_messages.size();
}

size_t MessageCounter::numErrors() const {
    return m_numErrors;
}

size_t MessageCounter::numWarnings() const {
    return m_numWarnings;
}

size_t MessageCounter::numNotes() const {
    return m_numNotes;
}

void MessageCounter::addMessage(const std::string& fileName,
                            int lineNumber,
                            Log::MessageType messageType,
                            const std::string& description) {

    if (includeMessage( messageType )) {
        switch (messageType) {
        case Log::Note:
            ++m_numNotes;
            break;

        case Log::Warning:
            ++m_numWarnings;
            break;

        case Log::Error:
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


void MessageCounter::addNote(const std::string& fileName,
                        int lineNumber,
                        const std::string& description) {
    addMessage(fileName, lineNumber, Log::Note, description);
}

void MessageCounter::addWarning(const std::string& fileName,
                           int lineNumber,
                           const std::string& description) {
    addMessage(fileName, lineNumber, Log::Warning, description);
}

void MessageCounter::addError(const std::string& fileName,
                         int lineNumber,
                         const std::string& description) {
    addMessage(fileName, lineNumber, Log::Error, description);
}

void MessageCounter::clear()
{
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    m_messages.clear();
}

void MessageCounter::append(const MessageCounter &other)
{
    for (size_t i = 0; i < other.size(); ++i) {
        addMessage(other.getFileName(i),
                   other.getLineNumber(i),
                   other.getMessageType(i),
                   other.getDescription(i));
    }
}

const std::string& MessageCounter::getFileName(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<0>(m_messages[msgIdx]);
}

int MessageCounter::getLineNumber(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<1>(m_messages[msgIdx]);
}

Log::MessageType MessageCounter::getMessageType(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<2>(m_messages[msgIdx]);
}

const std::string& MessageCounter::getDescription(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<3>(m_messages[msgIdx]);
}

const std::string MessageCounter::getFormattedMessage(size_t msgIdx) const {
    std::ostringstream oss;
    if (getLineNumber(msgIdx) > 0) {
        oss << getFileName(msgIdx) << ":"
            << getLineNumber(msgIdx) << ":";
    }

    switch (getMessageType(msgIdx)) {
    case Log::Note:
        oss << " note:";
        break;
    case Log::Warning:
        oss << " warning:";
        break;
    case Log::Error:
        oss << " error:";
        break;
    }
    oss << " " << getDescription(msgIdx);
    return oss.str();
}

void MessageCounter::printAll(std::ostream& os, size_t enabledTypes) const {
    for (size_t i = 0; i < size(); ++i)
        if (enabledTypes & getMessageType(i))
            os << getFormattedMessage(i) << "\n";
}

void MessageCounter::close() {
}



} // namespace Opm
