/*
  Copyright 2015 Statoil ASA.

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


#ifndef OPM_LOGBACKEND_HPP
#define OPM_LOGBACKEND_HPP

#include <opm/common/OpmLog/MessageFormatter.hpp>
#include <cstdint>
#include <string>
#include <memory>

namespace Opm
{

    /// Abstract interface class for log backends.
    class LogBackend
    {
    public:
        /// Construct with given message mask.
        explicit LogBackend(int64_t mask);

        /// Virtual destructor to enable inheritance.
        virtual ~LogBackend();

        /// Configure how decorateMessage() will modify message strings.
        void configureDecoration(std::shared_ptr<MessageFormatterInterface> formatter);

        /// Add a message to the backend.
        ///
        /// Typically a subclass may filter, change, and output
        /// messages based on configuration and the messageFlag.
        virtual void addMessage(int64_t messageFlag, const std::string& message) = 0;

        /// The message mask types are specified in the
        /// Opm::Log::MessageType namespace, in file LogUtils.hpp.
        int64_t getMask() const;

    protected:
        /// Return true if all bits of messageFlag are also set in our mask.
        bool includeMessage(int64_t messageFlag);

        /// Return decorated version of message depending on configureDecoration() arguments.
        std::string decorateMessage(int64_t messageFlag, const std::string& message);

    private:
        int64_t m_mask;
        std::shared_ptr<MessageFormatterInterface> m_formatter;
    };

} // namespace LogBackend


#endif
