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

#include <opm/parser/eclipse/Log/Logger.hpp>



namespace Opm {
Logger::Logger() {
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    setOutStream(NULL);
}

Logger::Logger(std::ostream* os) {
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    setOutStream(os);
}

void Logger::setOutStream(std::ostream* os) {
    m_outStream = os;
}

size_t Logger::size() const {
    return m_messages.size();
}

size_t Logger::numErrors() const {
    return m_numErrors;
}

size_t Logger::numWarnings() const {
    return m_numWarnings;
}

size_t Logger::numNotes() const {
    return m_numNotes;
}

void Logger::addMessage(const std::string& fileName,
                           int lineNumber,
                           MessageType messageType,
                           const std::string& description) {
    switch (messageType) {
    case Note:
        ++m_numNotes;
        break;

    case Warning:
        ++m_numWarnings;
        break;

    case Error:
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

void Logger::addNote(const std::string& fileName,
                        int lineNumber,
                        const std::string& description) {
    addMessage(fileName, lineNumber, Note, description);
}

void Logger::addWarning(const std::string& fileName,
                           int lineNumber,
                           const std::string& description) {
    addMessage(fileName, lineNumber, Warning, description);
}

void Logger::addError(const std::string& fileName,
                         int lineNumber,
                         const std::string& description) {
    addMessage(fileName, lineNumber, Error, description);
}

void Logger::clear()
{
    m_numErrors = 0;
    m_numWarnings = 0;
    m_numNotes = 0;

    m_messages.clear();
}

void Logger::append(const Logger &other)
{
    for (size_t i = 0; i < other.size(); ++i) {
        addMessage(other.getFileName(i),
                   other.getLineNumber(i),
                   other.getMessageType(i),
                   other.getDescription(i));
    }
}

const std::string& Logger::getFileName(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<0>(m_messages[msgIdx]);
}

int Logger::getLineNumber(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<1>(m_messages[msgIdx]);
}

Logger::MessageType Logger::getMessageType(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<2>(m_messages[msgIdx]);
}

const std::string& Logger::getDescription(size_t msgIdx) const {
    assert(msgIdx < size());
    return std::get<3>(m_messages[msgIdx]);
}

const std::string Logger::getFormattedMessage(size_t msgIdx) const {
    std::ostringstream oss;
    if (getLineNumber(msgIdx) > 0) {
        oss << getFileName(msgIdx) << ":"
            << getLineNumber(msgIdx) << ":";
    }

    switch (getMessageType(msgIdx)) {
    case Note:
        oss << " note:";
        break;
    case Warning:
        oss << " warning:";
        break;
    case Error:
        oss << " error:";
        break;
    }
    oss << " " << getDescription(msgIdx);
    return oss.str();
}

void Logger::printAll(std::ostream& os, size_t enabledTypes) const {
    for (size_t i = 0; i < size(); ++i)
        if (enabledTypes & getMessageType(i))
            os << getFormattedMessage(i) << "\n";
}

} // namespace Opm
