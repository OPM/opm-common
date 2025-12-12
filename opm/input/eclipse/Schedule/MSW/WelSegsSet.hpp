/*
  Copyright 2013 Statoil ASA.

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
#ifndef WEL_SEGS_SET_HPP
#define WEL_SEGS_SET_HPP

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

class Well;

class WelSegsSet
{
public:
    using Entry = std::pair<std::string,KeywordLocation>;

    void insert(const std::string& well_name,
                const KeywordLocation& location);

    std::vector<Entry> difference(const std::set<std::string>& compsegs,
                                  const std::vector<Well>& wells) const;
    std::vector<Entry> intersection(const std::set<std::string>& compsegs,
                                    const std::set<std::string>& comptraj) const;

private:
    struct PairComp
    {
        bool operator()(const Entry& pair, const std::string& str) const;
        bool operator()(const Entry& pair1, const Entry& pair2) const;
        bool operator()(const std::string& str, const Entry& pair) const;
    };

    std::set<Entry,PairComp> entries_;
};

} // end namespace Opm

#endif // WEL_SEGS_SET_HPP
