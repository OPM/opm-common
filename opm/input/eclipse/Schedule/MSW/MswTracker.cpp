/*
  Copyright 2026 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/MSW/MswTracker.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

bool Opm::MswTracker::isKnown(const std::string& well) const
{
    return this->index_.find(well) != this->index_.end();
}

void Opm::MswTracker::defined(const std::string& well)
{
    this->entryOrNew(well);
}

void Opm::MswTracker::seed(const Well& well)
{
    if (this->isKnown(well.name())) {
        return;
    }

    auto& e = this->entryOrNew(well.name());

    // Restart file information carries no keyword locations.
    const auto noLocation = KeywordLocation{};

    for (const auto& conn : well.getConnections()) {
        e.connections.try_emplace(conn.global_index(), noLocation);

        if (conn.attachedToSegment()) {
            e.links.insert_or_assign(conn.global_index(),
                                     LinkInfo { conn.segment(), noLocation });
        }
    }

    if (well.isMultiSegment()) {
        e.multiSegment = true;

        for (const auto& segment : well.getSegments()) {
            e.segments.insert(segment.segmentNumber());
        }
    }

    // Data from a restart file is assumed to be consistent.
    e.dirty = false;
}

void Opm::MswTracker::connections(const std::string&            well,
                                  const std::vector<CellIndex>& cells,
                                  const KeywordLocation&        location)
{
    auto* e = this->entry(well);
    if (e == nullptr) {
        return;
    }

    for (const auto& cell : cells) {
        if (e->connections.try_emplace(cell, location).second) {
            e->dirty = true;
        }
    }
}

void Opm::MswTracker::segments(const std::string&      well,
                               const std::vector<int>& segments)
{
    auto* e = this->entry(well);
    if (e == nullptr) {
        return;
    }

    e->multiSegment = true;
    e->dirty = true;

    e->segments.insert(segments.begin(), segments.end());
}

void Opm::MswTracker::linkConnections(const std::string&       well,
                                      const std::vector<Link>& links,
                                      const KeywordLocation&   location)
{
    auto* e = this->entry(well);
    if (e == nullptr) {
        return;
    }

    e->dirty = true;

    // Distinct segments, in order of appearance, for each cell in 'links'.
    auto cellSegments = std::map<CellIndex, std::vector<int>>{};
    for (const auto& [cell, segment] : links) {
        auto& segs = cellSegments[cell];
        if (std::find(segs.begin(), segs.end(), segment) == segs.end()) {
            segs.push_back(segment);
        }

        // Later links replace earlier ones--including those from this call.
        e->links.insert_or_assign(cell, LinkInfo { segment, location });
    }

    for (auto& [cell, segs] : cellSegments) {
        if (segs.size() > 1) {
            e->conflicts.push_back({ e->name, ProblemKind::MultiplyLinked,
                                     cell, std::move(segs), location });
        }
    }
}

std::vector<Opm::MswTracker::Problem>
Opm::MswTracker::checkAndClearDirty()
{
    auto problems = std::vector<Problem>{};

    for (auto& e : this->wells_) {
        if (! e.dirty) {
            continue;
        }

        e.dirty = false;

        auto conflicts = std::vector<Problem>{};
        conflicts.swap(e.conflicts);

        if (! e.multiSegment) {
            continue;
        }

        for (const auto& [cell, location] : e.connections) {
            if (e.links.find(cell) == e.links.end()) {
                problems.push_back({ e.name, ProblemKind::UnlinkedConnection,
                                     cell, {}, location });
            }
        }

        for (const auto& [cell, link] : e.links) {
            if (e.segments.find(link.segment) == e.segments.end()) {
                problems.push_back({ e.name, ProblemKind::UnknownSegment,
                                     cell, { link.segment }, link.location });
            }
        }

        std::move(conflicts.begin(), conflicts.end(),
                  std::back_inserter(problems));
    }

    return problems;
}

Opm::MswTracker::WellEntry*
Opm::MswTracker::entry(const std::string& well)
{
    const auto it = this->index_.find(well);
    if (it == this->index_.end()) {
        return nullptr;
    }

    return &this->wells_[it->second];
}

Opm::MswTracker::WellEntry&
Opm::MswTracker::entryOrNew(const std::string& well)
{
    const auto [it, inserted] = this->index_.try_emplace(well, this->wells_.size());
    if (inserted) {
        this->wells_.emplace_back().name = well;
    }

    return this->wells_[it->second];
}
