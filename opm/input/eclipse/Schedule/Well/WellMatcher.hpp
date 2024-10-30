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
#ifndef WELL_MATCHER_HPP
#define WELL_MATCHER_HPP

#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>
#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>

#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

class WellMatcher
{
public:
    /// Default constructor.
    WellMatcher() = default;

    /// Constructor.
    ///
    /// Assumes ownership of the NameOrder object.
    ///
    /// \param[in] well_order Facility for ordering a list of well names
    /// according to the associate insertion index.
    explicit WellMatcher(NameOrder&& well_order);

    /// Constructor.
    ///
    /// Does not assume ownership of the NameOrder object which, therefore,
    /// must outlive the WellMatcher object.  This is an optimisation for
    /// the common case of short-lived WellMatcher objects with backing
    /// store in a containing Schedule object.
    ///
    /// \param[in] well_order Facility for ordering a list of well names
    /// according to the associate insertion index.
    explicit WellMatcher(const NameOrder* well_order);

    /// Constructor.
    ///
    /// \param[in] wells List of wells for which to establish a matcher.
    explicit WellMatcher(std::initializer_list<std::string> wells);

    /// Constructor.
    ///
    /// \param[in] wells List of wells for which to establish a matcher.
    explicit WellMatcher(const std::vector<std::string>& wells);

    /// Constructor.
    ///
    /// Does not assume ownership of either the NameOrder or the
    /// WListManager objects.  Therefore, both must outlive the WellMatcher
    /// object.  This is an optimisation for the common case of short-lived
    /// WellMatcher objects with backing store in a containing Schedule
    /// object.
    ///
    /// \param[in] well_order Facility for ordering a list of well names
    /// according to the associate insertion index.
    ///
    /// \param[in] wlm Run's active well lists (WLIST keyword).
    WellMatcher(const NameOrder* well_order, const WListManager& wlm);

    /// Copy constructor.
    ///
    /// \param[in] rhs Source object from which to construct *this.
    WellMatcher(const WellMatcher& rhs);

    /// Move constructor
    ///
    /// \param[in,out] rhs Source object from which to construct *this.
    WellMatcher(WellMatcher&& rhs);

    /// Destructor.
    ~WellMatcher();

    /// Assignment operator
    ///
    /// \param[in] rhs Source object from which to retrieve object value.
    ///
    /// \return *this.
    WellMatcher& operator=(const WellMatcher& rhs);

    /// Move-assignment operator.
    ///
    /// \param[in,out] rhs Source object from which to retrieve object value.
    ///
    /// \return *this.
    WellMatcher& operator=(WellMatcher&& rhs);

    /// Sort a list of well names according to the established order.
    ///
    /// Throws an exception if any name in the list is not known.
    ///
    /// \param[in] wells List of well names.
    ///
    /// \return Well names in sorted order.
    std::vector<std::string> sort(std::vector<std::string> wells) const;

    /// Retrieve sorted list of well names matching a pattern.
    ///
    /// \param[in] pattern Well name, well template, well list name or well
    /// list template.
    ///
    /// \return List of unique well names matching \p pattern, sorted by
    /// name order.
    std::vector<std::string> wells(const std::string& pattern) const;

    /// Retrieve list of known wells.
    const std::vector<std::string>& wells() const;

private:
    // Note to maintainers: If you make any changes here, please carefully
    // update the constructors and assignment operators accordingly.

    /// Well name ordering object.
    ///
    /// Non-null only when taking ownership of an existing NameOrder object
    /// (NameOrder&& constructor), or when the WellMatcher is constructed
    /// from an explicit list of well names.
    ///
    /// Pay attention when implementing the special member functions.
    std::unique_ptr<NameOrder> m_wo{};

    /// Effective name ordering object.
    ///
    /// Either \code m_wo.get() \endcode, or the address of some external
    /// NameOrder object that outlives the WellMatcher.
    ///
    /// Pay attention when implementing the special member functions.
    const NameOrder* m_well_order{nullptr};

    /// Run's active well lists.
    ///
    /// Non-empty only if WellMatcher was formed by the two-argument
    /// constructor.
    ///
    /// Pay attention when implementing the special member functions.
    std::optional<std::reference_wrapper<const WListManager>> m_wlm{};
};

} // namespace Opm

#endif // WELL_MATCHER_HPP
