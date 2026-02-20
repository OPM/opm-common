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

#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>

#include <opm/common/utility/shmatch.hpp>

#include <opm/io/eclipse/rst/state.hpp>

#include <opm/input/eclipse/Schedule/Well/WList.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

    WListManager WListManager::serializationTestObject()
    {
        WListManager result;
        result.wlists = {{"test1", WList({"test2", "test3"}, "test1")}};

        return result;
    }

    WListManager::WListManager(const RestartIO::RstState& rst_state)
    {
        for (const auto& [wlist, well] : rst_state.wlists)
            this->newList(wlist, well);
    }

    std::size_t WListManager::WListSize() const
    {
        return (this->wlists.size());
    }

    bool WListManager::hasWell(const std::string& pattern) const
    {
        return std::any_of(this->wlists.begin(), this->wlists.end(),
                           [patt = pattern.substr(1)](const auto& wlist)
                           {
                               return shmatch(patt, wlist.first.substr(1))
                                   && !wlist.second.empty();
                           });
    }

    bool WListManager::hasList(const std::string& name) const
    {
        return this->wlists.find(name) != this->wlists.end();
    }

    WList& WListManager::newList(const std::string&              wlistName,
                                 const std::vector<std::string>& newWells)
    {
        if (this->hasList(wlistName)) {
            if (! newWells.empty()) {
                this->resetExistingWList(wlistName, newWells);
            }
            else {
                this->clearExistingWList(wlistName);
            }
        }
        else {
            this->createNewWList(wlistName, newWells);
        }

        return this->getList(wlistName);
    }

    WList& WListManager::getList(const std::string& name)
    {
        return this->wlists.at(name);
    }

    const WList& WListManager::getList(const std::string& name) const
    {
        return this->wlists.at(name);
    }

    const std::vector<std::string>&
    WListManager::getWListNames(const std::string& wname) const
    {
        return this->well_wlist_names.at(wname);
    }

    bool WListManager::hasWList(const std::string& wname) const
    {
        return this->well_wlist_names.find(wname)
            != this->well_wlist_names.end();
    }

    std::size_t WListManager::getNoWListsWell(const std::string& wname) const
    {
        return this->no_wlists_well.at(wname);
    }

    void WListManager::addWListWell(const std::string& wname, const std::string& wlname)
    {
        //add well to wlist if it is not already in the well list
        this->getList(wlname).add(wname);

        //add well list to well if not in vector already
        if (this->well_wlist_names.count(wname) > 0) {
            auto& no_wl = this->no_wlists_well.at(wname);
            auto& wlist_vec = this->well_wlist_names.at(wname);
            if (std::count(wlist_vec.begin(), wlist_vec.end(), wlname) == 0) {
                wlist_vec.push_back(wlname);
                no_wl += 1;
            }
        } else {
            //make wlist vector for new well
            std::vector<std::string> new_wlvec;
            std::size_t sz = 1;
            new_wlvec.push_back(wlname);
            this->well_wlist_names.insert({wname, new_wlvec});
            this->no_wlists_well.insert({wname, sz});
        }
    }

    void WListManager::addOrCreateWellList(const std::string& wlname, const std::vector<std::string>& wnames)
    {
        if (!this->hasList(wlname)) {
            this->newList(wlname, wnames);
        } else {
            for (const auto& wname : wnames) {
                this->addWListWell(wname, wlname);
            }
        }
    }

    bool WListManager::delWell(const std::string& wname)
    {
        auto well_lists_changed = false;

        for (auto& pair: this->wlists) {
            auto& wlist = pair.second;
            wlist.del(wname);
            if (this->well_wlist_names.count(wname) > 0) {
                auto& wlist_vec = this->well_wlist_names.at(wname);
                auto& no_wl = this->no_wlists_well.at(wname);
                auto itwl = std::find(wlist_vec.begin(), wlist_vec.end(), wlist.getName());
                if (itwl != wlist_vec.end()) {
                    wlist_vec.erase(itwl);
                    no_wl -= 1;
                    if (no_wl == 0) {
                        wlist_vec.clear();
                    }

                    well_lists_changed = true;
                }
            }
        }

        return well_lists_changed;
    }

    bool WListManager::delWListWell(const std::string& wname,
                                    const std::string& wlname)
    {
        //delete well from well list
        this->getList(wlname).del(wname);

        if (this->well_wlist_names.count(wname) > 0) {
            auto& wlist_vec = this->well_wlist_names.at(wname);
            auto& no_wl = this->no_wlists_well.at(wname);
            // reduce the no of well lists associated with a well, delete whole list if no wlists is zero
            const auto& it = std::find(wlist_vec.begin(), wlist_vec.end(), wlname);
            if (it != wlist_vec.end()) {
                no_wl -= 1;
                if (no_wl == 0) {
                    wlist_vec.clear();
                }

                return true;
            }
        }

        return false;
    }

    bool WListManager::operator==(const WListManager& data) const
    {
        return this->wlists == data.wlists;
    }

    std::vector<std::string>
    WListManager::wells(const std::string& wlist_pattern) const
    {
        if (const auto wlistPos = this->wlists.find(wlist_pattern);
            wlistPos != this->wlists.end())
        {
            return wlistPos->second.wells();
        }

        auto allWells = std::vector<std::string>{};

        const auto pattern = wlist_pattern.substr(1);
        for (const auto& [name, wlist] : this->wlists) {
            if (! shmatch(pattern, name.substr(1))) {
                continue;
            }

            const auto& well_names = wlist.wells();
            allWells.insert(allWells.end(), well_names.begin(), well_names.end());
        }

        if (allWells.empty()) {
            // No active wells in any of the well lists matching
            // 'wlist_pattern' (or no well lists matching that pattern).
            // Return empty list of well names.
            return allWells;
        }

        // Prune duplicate well names.  Uses sorted indices into 'allWells'.
        auto wellIx = std::vector<std::vector<std::string>::size_type>(allWells.size());
        std::iota(wellIx.begin(), wellIx.end(), std::vector<std::string>::size_type{0});

        std::ranges::sort(wellIx,
                          [&allWells](const auto i1, const auto i2)
                          { return allWells[i1] < allWells[i2]; });

        wellIx.erase(std::unique(wellIx.begin(), wellIx.end(),
                                 [&allWells](const auto i1, const auto i2)
                                 { return allWells[i1] == allWells[i2]; }),
                     wellIx.end());

        if (wellIx.size() == allWells.size()) {
            // AllWells holds unique well names only.
            return allWells;
        }

        // Re-sort unique well names based on order of appearance.
        std::ranges::sort(wellIx);

        auto uniqueWells = std::vector<std::string>(wellIx.size());
        std::ranges::transform(wellIx, uniqueWells.begin(),
                               [&allWells](const auto i) { return allWells[i]; });

        return uniqueWells;
    }

    void WListManager::resetExistingWList(const std::string&              wlistName,
                                          const std::vector<std::string>& newWells)
    {
        // new well list contains wells

        // Existing wells in 'wlistName' that are not in 'newWells'.
        auto deleteWells = std::vector<std::string>{};

        {
            auto wlistWells = this->getList(wlistName).wells();
            std::ranges::sort(wlistWells);

            auto newWellsCpy = newWells;
            std::ranges::sort(newWellsCpy);

            std::set_difference(wlistWells.begin(), wlistWells.end(),
                                newWellsCpy.begin(), newWellsCpy.end(),
                                std::back_inserter(deleteWells));
        }

        for (const auto& deleteWell : deleteWells) {
            this->delWListWell(deleteWell, wlistName);
        }

        this->getList(wlistName).clear();

        for (const auto& wname : newWells) {
            // add wells on new wlist
            this->addWListWell(wname, wlistName);
        }
    }

    void WListManager::clearExistingWList(const std::string& wlistName)
    {
        // Remove all wells from existing well list (empty WLIST NEW)

        // Intentional copy so that name removal does not interfere with
        // iteration.
        const auto wlistWells = this->getList(wlistName).wells();

        for (const auto& wname : wlistWells) {
            this->delWListWell(wname, wlistName);
        }
    }

    void WListManager::createNewWList(const std::string&              wlistName,
                                      const std::vector<std::string>& newWells)
    {
        // create a new wlist (new well list name)
        this->wlists.try_emplace(wlistName, std::vector<std::string>{}, wlistName);

        for (const auto& wname : newWells) {
            this->addWListWell(wname, wlistName);
        }
    }
}
