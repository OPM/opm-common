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
#ifndef WLISTMANAGER_HPP
#define WLISTMANAGER_HPP

#include <opm/input/eclipse/Schedule/Well/WList.hpp>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace Opm::RestartIO {
    struct RstState;
} // namespace Opm::RestartIO

namespace Opm {

class WListManager
{
public:
    WListManager() = default;
    explicit WListManager(const RestartIO::RstState& rst_state);
    static WListManager serializationTestObject();

    std::size_t WListSize() const;
    bool hasList(const std::string&) const;
    WList& getList(const std::string& name);
    const WList& getList(const std::string& name) const;
    WList& newList(const std::string& name, const std::vector<std::string>& wname);

    const std::vector<std::string>& getWListNames(const std::string& wname) const;
    std::size_t getNoWListsWell(const std::string& wname) const;
    bool hasWList(const std::string& wname) const;
    void addWListWell(const std::string& wname, const std::string& wlname);
    void delWell(const std::string& wname);
    void delWListWell(const std::string& wname, const std::string& wlname);

    bool operator==(const WListManager& data) const;
    std::vector<std::string> wells(const std::string& wlist_pattern) const;
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(wlists);
        serializer(well_wlist_names);
        serializer(no_wlists_well);
    }

private:
    std::map<std::string, WList> wlists;
    std::map<std::string, std::vector<std::string>> well_wlist_names;
    std::map<std::string, std::size_t> no_wlists_well;

    /// Reset contents of existing well list.
    ///
    /// Implements the 'NEW' operation with a non-empty list of wells for
    /// the case of an existing well list.  On exit, the existing well list
    /// object will hold only those wells that are include in the 'NEW'
    /// operation.
    ///
    /// \param[in] wlistName Well list name.
    ///
    /// \param[in] newWells List of wells to include in the new well list
    /// object.
    void resetExistingWList(const std::string& wlistName,
                            const std::vector<std::string>& newWells);

    /// Clear contents of existing well list.
    ///
    /// Implements the 'NEW' operation for a empty list of wells in the case
    /// of an existing well list.
    ///
    /// \param[in] wlistName Well list name.
    void clearExistingWList(const std::string& wlistName);

    /// Create a new well list.
    ///
    /// Implements the 'NEW' operation with a non-empty list of wells for
    /// the case of a non-existent well list.
    ///
    /// \param[in] wlistName Well list name.
    ///
    /// \param[in] newWells List of wells to include in the new well list
    /// object.
    void createNewWList(const std::string& wlistName,
                        const std::vector<std::string>& newWells);
};

} // namespace Opm

#endif // WLISTMANAGER_HPP
