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
#include <fnmatch.h>

#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <optional>

#include <opm/parser/eclipse/EclipseState/Schedule/Well/WList.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WListManager.hpp>
namespace Opm {

    WListManager WListManager::serializeObject()
    {
        WListManager result;
        result.wlists = {{"test1", WList({"test2", "test3"})}};

        return result;
    }

    std::size_t WListManager::WListSize() const {
        return (this->wlists.size());
    }

    bool WListManager::hasList(const std::string& name) const {
        return (this->wlists.find(name) != this->wlists.end());
    }


    WList& WListManager::newList(const std::string& name) {
        this->wlists.erase(name);
        this->wlists.insert( {name, WList() });
        return this->getList(name);
    }


    WList& WListManager::getList(const std::string& name) {
        return this->wlists.at(name);
    }

    const WList& WListManager::getList(const std::string& name) const {
        return this->wlists.at(name);
    }
#if 0
    void WListManager::delWell(const std::string& well) {
        for (auto& pair: this->wlists) {
            auto& wlist = pair.second;
            wlist.del(well);
        }
    }
#endif
    const std::vector<std::string>& WListManager::getWListNames(const std::string& wname) const {
            return this->well_wlist_names.at(wname);
    }

    bool WListManager::hasWList(const std::string& wname) const {
        if (this->well_wlist_names.count(wname) > 0) {
            return true;
        } else {
            return false;
        }
    }

    void WListManager::addWListWell(const std::string& wname, const std::string& wlname) {
        //add well to wlist if it is not already in the well list
        auto& wlist = this->getList(wlname);
        wlist.add(wname);
        //add well list to well if not in vector already
        if (this->well_wlist_names.count(wname) > 0) {
            auto& wlist_vec = this->well_wlist_names.at(wname);
            if (std::count(wlist_vec.begin(), wlist_vec.end(), wlname) == 0)
                wlist_vec.push_back(wlname);
        } else {
            //make wlist vector for new well
            std::vector<std::string> new_wlvec;
            new_wlvec.push_back(wlname);
            this->well_wlist_names.insert({wname, new_wlvec});
        }
    }

    void WListManager::delWell(const std::string& wname) {
        for (auto& pair: this->wlists) {
            auto& wlist = pair.second;
            wlist.del(wname);
        }
        const auto& it = this->well_wlist_names.find(wname);
        this->well_wlist_names.erase(it);
    }

    void WListManager::delWListWell(const std::string& wname, const std::string& wlname) {
        //delete well from well list
        auto& wlist = this->getList(wlname);
        wlist.del(wname);
        if (this->well_wlist_names.count(wname) > 0) {
            auto& wlist_vec = this->well_wlist_names.at(wname);
            // remove wlist element from vector of well lists for a well
            const auto& it = std::find(wlist_vec.begin(), wlist_vec.end(), wlname);
            if (it != wlist_vec.end()) {
                wlist_vec.erase(it);
            }
        }
    }

    bool WListManager::operator==(const WListManager& data) const {
        return this->wlists == data.wlists;
    }

    std::vector<std::string> WListManager::wells(const std::string& wlist_pattern) const {
        if (this->hasList(wlist_pattern)) {
            const auto& wlist = this->getList(wlist_pattern);
            return { wlist.wells() };
        } else {
            std::vector<std::string> well_set;
            auto pattern = wlist_pattern.substr(1);
            for (const auto& [name, wlist] : this->wlists) {
                auto wlist_name = name.substr(1);
                int flags = 0;
                if (fnmatch(pattern.c_str(), wlist_name.c_str(), flags) == 0) {
                    const auto& well_names = wlist.wells();
                    for ( auto it = well_names.begin(); it != well_names.end(); it++ ) {
                       if (std::count(well_set.begin(), well_set.end(), *it) == 0)
                           well_set.push_back(*it);
                    }
                }
            }
            return { well_set };
        }
    }

}
