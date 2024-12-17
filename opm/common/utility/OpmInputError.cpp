/*
  Copyright 2020 Equinor ASA.

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

#include <opm/common/utility/OpmInputError.hpp>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {

template <typename... Args>
std::string formatImpl(std::string_view            msg_format,
                       const Opm::KeywordLocation& loc,
                       Args&&...                   arguments)
{
    return fmt::format(fmt::runtime(msg_format),
                       std::forward<Args>(arguments)...,
                       fmt::arg("keyword", loc.keyword),
                       fmt::arg("file", loc.filename),
                       fmt::arg("line", loc.lineno));
}

} // Anonymous namespace

std::string
Opm::OpmInputError::formatException(const std::exception&  e,
                                    const KeywordLocation& loc)
{
    return formatImpl(R"(Problem with keyword {keyword}
In {file} line {line}.
{0})",
                      loc, e.what());
}

// Note: fmt::format() is a variadic template whence it is possible to
// forward arbitrary arguments directly to this function.  On the other
// hand, using that ability will make OpmInputError::format() a function
// template in a header too and, moreover, confer the fmtlib prerequiste
// onto downstream modules.
std::string
Opm::OpmInputError::format(const std::string&     msg_format,
                           const KeywordLocation& loc)
{
    return formatImpl(msg_format, loc);
}

std::string
Opm::OpmInputError::formatSingle(const std::string&     reason,
                                 const KeywordLocation& location)
{
    return formatImpl(R"(Problem with keyword {keyword}
In {file} line {line}
{0})",
                      location, reason);
}

namespace {

std::string locationStringLine(const Opm::KeywordLocation& loc)
{
    return Opm::OpmInputError::format("\n  {keyword} in {file}, line {line}", loc);
}

} // Anonymous namespace

std::string
Opm::OpmInputError::formatMultiple(const std::string&                  reason,
                                   const std::vector<KeywordLocation>& locations)
{
    const auto messages =
        std::accumulate(locations.begin(), locations.end(), std::string{},
                        [](const std::string& s, const KeywordLocation& loc)
                        { return s + locationStringLine(loc); });

    return fmt::format(R"(Problem with keywords {}
{})",
                       messages, reason);
}
