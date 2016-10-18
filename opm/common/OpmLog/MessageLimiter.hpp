/*
  Copyright 2016 SINTEF ICT, Applied Mathematics.

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

#ifndef OPM_MESSAGELIMITER_HEADER_INCLUDED
#define OPM_MESSAGELIMITER_HEADER_INCLUDED

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <opm/common/OpmLog/LogUtil.hpp>
namespace Opm
{


    /// Handles limiting the number of messages with the same tag.
    class MessageLimiter
    {
    public:
        /// Used to indicate no message number limit.
        enum { NoLimit = -1 };

        /// Default constructor, no limit to the number of messages.
        MessageLimiter()
            : tag_limit_(NoLimit),
              category_limits_({{Log::MessageType::Note, NoLimit},
                                {Log::MessageType::Info, NoLimit},
                                {Log::MessageType::Warning, NoLimit},
                                {Log::MessageType::Error, NoLimit},
                                {Log::MessageType::Problem, NoLimit},
                                {Log::MessageType::Bug, NoLimit}})
        {
        }

        /// Construct with given limit to number of messages with the
        /// same tag.
        ///
        /// Negative limits (including NoLimit) are interpreted as
        /// NoLimit, but the default constructor is the preferred way
        /// to obtain that behaviour.
        explicit MessageLimiter(const int tag_limit)
            : tag_limit_(tag_limit < 0 ? NoLimit : tag_limit),
              category_limits_({{Log::MessageType::Note, NoLimit},
                                {Log::MessageType::Info, NoLimit},
                                {Log::MessageType::Warning, NoLimit},
                                {Log::MessageType::Error, NoLimit},
                                {Log::MessageType::Problem, NoLimit},
                                {Log::MessageType::Bug, NoLimit}})
        {
        }

        MessageLimiter(const int tag_limit, const std::map<int64_t, int> category_limits)
            : tag_limit_(tag_limit < 0 ? NoLimit : tag_limit),
              category_limits_(category_limits)
        {
        }

        /// The message limit (same for all tags).
        int messageLimit() const
        {
            return tag_limit_;
        }

        /// Used for handleMessageLimits() return type (see that
        /// function).
        enum class Response
        {
            PrintMessage, JustOverTagLimit, JustOverCategoryLimit, OverTagLimit, OverCategoryLimit
        };

        /// If a tag is empty, there is no message limit or for that
        /// (count <= limit), respond PrintMessage.
        /// If (count == limit + 1), respond JustOverLimit.
        /// If (count > limit + 1), respond OverLimit.
        Response handleMessageLimits(const std::string& tag, const int64_t messageMask)
        {
            Response res = Response::PrintMessage;
            // Deal with tag limits.
            if (tag.empty() || tag_limit_ == NoLimit) {
                category_counts_[messageMask]++;
                res = Response::PrintMessage;
            } else {
                // See if tag already encountered.
                auto it = tag_counts_.find(tag);
                if (it != tag_counts_.end()) {
                    // Already encountered this tag. Increment its count.
                    const int count = ++it->second;
                    res = countBasedResponse(count, messageMask);
                } else {
                    // First encounter of this tag. Insert 1.
                    tag_counts_.insert({tag, 1});
                    res = countBasedResponse(1, messageMask);
                }
            }
            // If tag count reached the limit, ignore all the same tagged messages.
            if (res == Response::JustOverTagLimit || res == Response::OverTagLimit) {
                return res;
            } else {
                if (category_limits_[messageMask] == NoLimit) {
                    category_counts_[messageMask]++;
                    res = Response::PrintMessage;
                } else {
                    res = countBasedResponse(messageMask);
                }
            }
            return res;
        }

    private:
        Response countBasedResponse(const int count, const int64_t messageMask)
        {
            if (count <= tag_limit_) {
                category_counts_[messageMask]++;
                return Response::PrintMessage;
            } else if (count == tag_limit_ + 1) {
                category_counts_[messageMask]++;
                return Response::JustOverTagLimit;
            } else {
                return Response::OverTagLimit;
            }
        }


        Response countBasedResponse(const int64_t messageMask)
        {
            if (category_counts_[messageMask] < category_limits_[messageMask]) {
                category_counts_[messageMask]++;
                return Response::PrintMessage;
            } else if (category_counts_[messageMask] == category_limits_[messageMask] + 1) {
                category_counts_[messageMask]++;
                return Response::JustOverCategoryLimit;
            } else {
                return Response::OverCategoryLimit;
            }
        }

        int tag_limit_;
        std::unordered_map<std::string, int> tag_counts_;
        std::map<int64_t, int> category_counts_;
        // info, note, warning, problem, error, bug.
        std::map<int64_t, int> category_limits_;
        bool reachTagLimit_;
    };



} // namespace Opm

#endif // OPM_MESSAGELIMITER_HEADER_INCLUDED
