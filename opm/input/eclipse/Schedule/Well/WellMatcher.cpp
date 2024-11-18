/*
  Copyright 2019 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>

#include <opm/common/utility/shmatch.hpp>

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
    const std::vector<std::string>& emptyWellList()
    {
        static const auto wlist = std::vector<std::string>{};
        return wlist;
    }

    std::string normalisePattern(const std::string& patt)
    {
        if (patt.front() == '\\') {
            // Trim leading '\' character since the 'patt' might be
            // something like
            //
            //    '\*P*' or '\?????'
            //
            // which denote, respectively, all wells (typically) whose names
            // contain at least one 'P' anywhere in the name or all wells or
            // groups whose names have exactly five characters.  Without the
            // leading backslash, the first pattern would match all well
            // lists whose names begin with 'P' and the second might be
            // misconstrued as the '?' pattern matching all wells for which
            // an ACTIONX condition is true.
            return patt.substr(1);
        }

        return patt;
    }
} // Anonymous namespace

Opm::WellMatcher::WellMatcher(NameOrder&& well_order)
    : m_wo         { std::make_unique<NameOrder>(std::move(well_order)) }
    , m_well_order { m_wo.get() }
{}

Opm::WellMatcher::WellMatcher(const NameOrder* well_order)
    : m_wo{}
    , m_well_order { well_order }
{}

Opm::WellMatcher::WellMatcher(std::initializer_list<std::string> wells)
    : m_wo         { std::make_unique<NameOrder>(wells) }
    , m_well_order { m_wo.get() }
{}

Opm::WellMatcher::WellMatcher(const std::vector<std::string>& wells)
    : m_wo         { std::make_unique<NameOrder>(wells) }
    , m_well_order { m_wo.get() }
{}

Opm::WellMatcher::WellMatcher(const NameOrder*    well_order,
                              const WListManager& wlm)
    : m_wo         {}
    , m_well_order { well_order }
    , m_wlm        { std::in_place, wlm }
{}

Opm::WellMatcher::WellMatcher(const WellMatcher& rhs)
    : m_wo         {}
    , m_well_order { rhs.m_well_order }
{
    if (rhs.m_wo != nullptr) {
        this->m_wo = std::make_unique<NameOrder>(*rhs.m_wo);
        this->m_well_order = this->m_wo.get();
    }

    if (rhs.m_wlm.has_value()) {
        this->m_wlm.emplace(rhs.m_wlm->get());
    }
}

Opm::WellMatcher::WellMatcher(WellMatcher&& rhs)
    : m_wo         { std::move(rhs.m_wo) }
    , m_well_order { rhs.m_well_order }
{
    if (this->m_wo != nullptr) {
        this->m_well_order = this->m_wo.get();
    }

    if (rhs.m_wlm.has_value()) {
        this->m_wlm.emplace(rhs.m_wlm->get());
    }
}

Opm::WellMatcher::~WellMatcher() = default;

Opm::WellMatcher& Opm::WellMatcher::operator=(const WellMatcher& rhs)
{
    if (this == &rhs) {
        return *this;
    }

    if (rhs.m_wo != nullptr) {
        this->m_wo = std::make_unique<NameOrder>(*rhs.m_wo);
    }

    this->m_well_order = (this->m_wo != nullptr)
        ? this->m_wo.get() : rhs.m_well_order;

    if (rhs.m_wlm.has_value()) {
        this->m_wlm.emplace(rhs.m_wlm->get());
    }

    return *this;
}

Opm::WellMatcher& Opm::WellMatcher::operator=(WellMatcher&& rhs)
{
    if (this == &rhs) {
        return *this;
    }

    this->m_wo = std::move(rhs.m_wo);
    this->m_well_order = (this->m_wo != nullptr)
        ? this->m_wo.get() : rhs.m_well_order;

    if (rhs.m_wlm.has_value()) {
        this->m_wlm.emplace(rhs.m_wlm->get());
    }

    return *this;
}

std::vector<std::string>
Opm::WellMatcher::sort(std::vector<std::string> wells) const
{
    return (this->m_well_order != nullptr)
        ? this->m_well_order->sort(std::move(wells))
        : std::move(wells);
}

const std::vector<std::string>& Opm::WellMatcher::wells() const
{
    return (this->m_well_order != nullptr)
        ? this->m_well_order->names()
        : emptyWellList();
}

std::vector<std::string>
Opm::WellMatcher::wells(const std::string& pattern) const
{
    if (pattern.empty() || (this->m_well_order == nullptr)) {
        return {};
    }

    // WLIST
    if ((pattern.front() == '*') && (pattern.size() > 1)) {
        return this->m_wlm.has_value()
            ? this->sort(this->m_wlm->get().wells(pattern))
            : std::vector<std::string> {};
    }

    const auto patt = normalisePattern(pattern);

    // Normal pattern matching
    if (patt.find('*') != std::string::npos) {
        auto names = std::vector<std::string> {};
        names.reserve(this->m_well_order->size());

        std::copy_if(this->m_well_order->begin(),
                     this->m_well_order->end(),
                     std::back_inserter(names),
                     [&patt](const auto& wname)
                     { return shmatch(patt, wname); });

        names.shrink_to_fit();
        return names;
    }

    if (this->m_well_order->has(patt)) {
        return { patt };
    }

    return {};
}
