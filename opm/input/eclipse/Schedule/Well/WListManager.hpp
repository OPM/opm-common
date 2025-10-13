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

/// Collection of run's known well lists
///
/// Manages how well lists are created (NEW), how new wells are added to new
/// or existing lists (ADD), how wells move between well lists (MOV), and
/// how to remove a set of wells from an existing well list (DEL).
class WListManager
{
public:
    /// Default constructor.
    WListManager() = default;

    /// Constructor.
    ///
    /// Forms collection of well lists from restart file information.
    ///
    /// \param[in] rst_state Restart file information.
    explicit WListManager(const RestartIO::RstState& rst_state);

    /// Create a serialisation test object.
    static WListManager serializationTestObject();

    /// Number of well lists in current collection.
    std::size_t WListSize() const;

    /// Whether or not one or more wells matching a well list name or well
    /// list template exists.
    ///
    /// This predicate checks whether or not a well list exists matching the
    /// pattern *and* that that well list is non-empty.
    ///
    /// \param[in] pattern Well list name or well list template.
    ///
    /// \return Whether or not there are any current wells matching \p
    /// pattern.
    bool hasWell(const std::string& pattern) const;

    /// Predicate for existence of particular well lists
    ///
    /// \param[in] wlistName Well list name, including leading asterisk.
    ///
    /// \return Whether or not well list named \p wlistName exists in
    /// current collection.
    bool hasList(const std::string& wlistName) const;

    /// Access individual well list by name.
    ///
    /// Mutable version.
    ///
    /// Will throw an exception if the well list does not exist.  Use
    /// predicate hasList() to check for existence before using this
    /// function.
    ///
    /// \param[in] name Well list name, including the leading asterisk.
    ///
    /// \return Mutable well list named \p name.
    WList& getList(const std::string& name);

    /// Access individual well list by name.
    ///
    /// Read only version.
    ///
    /// Will throw an exception if the well list does not exist.  Use
    /// predicate hasList() to check for existence before using this
    /// function.
    ///
    /// \param[in] name Well list name, including the leading asterisk.
    ///
    /// \return Immutable well list named \p name.
    const WList& getList(const std::string& name) const;

    /// Create a new well list with initial sequence of wells
    ///
    /// Implements the NEW operation.
    ///
    /// \param[in] name Well list name, including the leading asterisk.
    ///
    /// \param[in] wname Initial collection of wells in well list \p name.
    ///
    /// \return New well list \p name.
    WList& newList(const std::string&              name,
                   const std::vector<std::string>& wname);

    /// Sequence of well lists containing named well.
    ///
    /// \param[in] wname Well name.
    ///
    /// \return Sequence of well lists containing well \p wname.
    const std::vector<std::string>&
    getWListNames(const std::string& wname) const;

    /// Number of well lists containing named well.
    ///
    /// \param[in] wname Well name.
    ///
    /// \return Number of well lists containing well \p wname.
    std::size_t getNoWListsWell(const std::string& wname) const;

    /// Whether or not named well is on any current well list.
    ///
    /// \param[in] wname Well name.
    ///
    /// \return Whether or not well \p wname is on any well list.
    bool hasWList(const std::string& wname) const;

    /// Add named well to named well list
    ///
    /// Will throw an exception if the well list does not already exist.
    ///
    /// \param[in] wname Well name.
    ///
    /// \param[in] wlname Well list name, including leading asterisk.
    void addWListWell(const std::string& wname, const std::string& wlname);

    /// Add sequence of wells to a named well list.
    ///
    /// Will create the well list if it does not already exist.  Implements
    /// the ADD operation.
    ///
    /// \param[in] wlname Well list name, including the leading asterisk.
    ///
    /// \param[in] wnames Sequence of named wells that will be added to well
    /// list \p wlname.
    void addOrCreateWellList(const std::string&              wlname,
                             const std::vector<std::string>& wnames);

    /// Remove named well from all existing well lists.
    ///
    /// No change if named well is not on any existing well list.
    /// Implements the DEL operation.
    ///
    /// \param[in] wname Well name.
    ///
    /// \return Whether or not any well list changed as a result of removing
    /// well \p wname.
    bool delWell(const std::string& wname);

    /// Remove named well from specific, named well list.
    ///
    /// \param[in] wname Named well.
    ///
    /// \param[in] wlname Well list name, including leading asterisk.
    ///
    /// \return Whether or not \p wlname changed.
    bool delWListWell(const std::string& wname, const std::string& wlname);

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const WListManager& data) const;

    /// All wells on all well lists matching a name pattern.
    ///
    /// Well names are unique.
    ///
    /// \param[in] wlist_pattern Well list name or well list name pattern.
    /// Should include the leading asterisk.
    ///
    /// \return Unique well names from all well lists matching the \p
    /// wlist_pattern.  Empty if no such list exists, or if all matching
    /// well lists are empty.
    std::vector<std::string> wells(const std::string& wlist_pattern) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(wlists);
        serializer(well_wlist_names);
        serializer(no_wlists_well);
    }

private:
    /// Current collection of well lists.
    ///
    /// Keyed by well list name.
    std::map<std::string, WList> wlists;

    /// Well lists containing named wells.
    ///
    /// Keyed by well name.
    std::map<std::string, std::vector<std::string>> well_wlist_names;

    /// Number of well lists containing named wells
    ///
    /// Keyed by well name.
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
