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

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>

namespace Opm
{


    /// Handles limiting the number of messages with the same tag.
    class MessageLimiter
    {
    public:
        /// Used to indicate no message number limit.
        enum { NoLimit = -1 };

        /// Used for handleMessageLimits() return type (see that function).
        enum class Response
        {
            /// Message should be printed.  Not affected by any limit.
            PrintMessage,

            /// Message has reached the current limit for this tag.
            JustOverTagLimit,

            /// Message has reached the current limit for this category.
            JustOverCategoryLimit,

            /// Message is over the current limit for this tag.
            OverTagLimit,

            /// Message is over the current limit of this category.
            OverCategoryLimit,
        };

        /// Default constructor, no limit to the number of messages.
        MessageLimiter() = default;

        /// Construct with given limit to number of messages with the
        /// same tag.
        ///
        /// Negative limits (including NoLimit) are interpreted as
        /// NoLimit, but the default constructor is the preferred way
        /// to obtain that behaviour.
        explicit MessageLimiter(const int tag_limit)
            : MessageLimiter { tag_limit, {} }
        {}

        MessageLimiter(const int tag_limit, const std::map<std::int64_t, int>& category_limits)
            : tag_limit_       { (tag_limit < 0) ? NoLimit : tag_limit }
            , category_limits_ { category_limits }
        {}

        /// The tag message limit (same for all tags).
        int tagMessageLimit() const
        {
            return tag_limit_;
        }

        /// If (tag count == tag limit + 1) for the passed tag, respond JustOverTagLimit.
        /// If (tag count > tag limit + 1), respond OverTagLimit.
        /// If a tag is empty, there is no tag message limit or for that tag
        /// (tag count <= tag limit), consider the category limits:
        /// If (category count == category limit + 1) for the passed messageMask, respond JustOverCategoryLimit.
        /// If (category count > category limit + 1), respond OverCategoryLimit.
        /// If (category count <= category limit), or there is no limit for that category, respond PrintMessage.
        Response handleMessageLimits(const std::string& tag,
                                     const std::int64_t messageMask) const
        {
            Response res = Response::PrintMessage;

            // Deal with tag limits.
            if (!tag.empty() && (this->tag_limit_ != NoLimit)) {
                res = this->countBasedResponseTag(this->increaseTagCount(tag));
            }

            if (res != Response::PrintMessage) {
                // Tag count reached limit.  Do not include message in
                // category count.
                return res;
            }

            // Tag count within limits.  Include message in category count.
            const auto count = this->increaseCategoryCount(messageMask);

            if (const auto limitPos = this->category_limits_.find(messageMask);
                (limitPos != this->category_limits_.end()) &&
                (limitPos->second != NoLimit))
            {
                // There is a defined category limit for 'messageMask'.
                // Generate response based on this limit.
                res = this->countBasedResponseCategory(count, limitPos->second);
            }

            // PrintMessage or category based response.
            return res;
        }

        /// Retrieve message count for specific category.
        ///
        /// Mostly provided for unit testing.
        ///
        /// \param[in] category Message category.
        ///
        /// \return Current message counts for \p category.
        int categoryMessageCount(const std::int64_t category) const
        {
            const auto countPos = this->category_counts_.find(category);

            return (countPos != this->category_counts_.end())
                ? countPos->second : 0;
        }

    private:
        /// Run's limit for tagged messages.
        ///
        /// Default unlimited.
        int tag_limit_ {NoLimit};

        /// Run's limit for built-in message categories.
        ///
        /// Influenced by the MESSAGES keyword.  No defined limit for a
        /// particular message category treated as NoLimit for that
        /// category.
        std::map<std::int64_t, int> category_limits_{};

        /// Message counts for user-defined message tags.
        mutable std::unordered_map<std::string, int> tag_counts_{};

        /// Message counts for built-in message categories.
        mutable std::map<std::int64_t, int> category_counts_{};

        int increaseTagCount(const std::string& tag) const
        {
            return increaseCount(tag, this->tag_counts_);
        }

        int increaseCategoryCount(const std::int64_t messageMask) const
        {
            return increaseCount(messageMask, this->category_counts_);
        }

        Response countBasedResponseTag(const int count) const
        {
            return response(count, this->tag_limit_,
                            Response::JustOverTagLimit,
                            Response::OverTagLimit);
        }

        Response countBasedResponseCategory(const int count,
                                            const int category_limit) const
        {
            return response(count, category_limit,
                            Response::JustOverCategoryLimit,
                            Response::OverCategoryLimit);
        }

        template <typename Key, class CountMap>
        static int increaseCount(const Key& key, CountMap& counts)
        {
            return ++counts.try_emplace(key, 0).first->second;
        }

        static Response response(const int      count,
                                 const int      limit,
                                 const Response justOverLimit,
                                 const Response overLimit)
        {
            if (count <= limit) {
                return Response::PrintMessage;
            } else if (count == limit + 1) {
                return justOverLimit;
            } else {
                return overLimit;
            }
        }
    };

} // namespace Opm

#endif // OPM_MESSAGELIMITER_HEADER_INCLUDED
