/*
  Copyright 2018 Equinor ASA.

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

#ifndef ActionContext_HPP
#define ActionContext_HPP

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace Opm {

class SummaryState;
class WListManager;

} // namespace Opm

namespace Opm::Action {

/// Manager of summary vector values.
///
/// Mainly a small wrapper around a SummaryState object.
class Context
{
public:
    /// Constructor.
    ///
    /// \param[in] summary_state Run's current summary vectors.
    ///
    /// \param[in] wlm Run's active well lists (WLIST keyword).
    explicit Context(const SummaryState& summary_state,
                     const WListManager& wlm);

    /// Assign function value for named entity.
    ///
    /// \param[in] func Named summary function, e.g., WOPR, GWCT, or WURST.
    ///
    /// \param[in] arg Object for which to retrieve function value, e.g., a
    /// well name.
    ///
    /// \param[in] value Numeric function value for \p func associated to
    /// named entity \p arg.
    void add(std::string_view func, std::string_view arg, double value);

    /// Assign function value.
    ///
    /// \param[in] key Combined key for a unique summary vector, e.g.,
    /// WOPR:PROD1, GGOR:FIELD, or SUBUNIT:PROD1:42.
    ///
    /// \param[in] value Numeric function value.
    void add(const std::string& key, double value);

    /// Retrieve function value (e.g., WOPR) for a specific entity.
    ///
    /// \param[in] func Named summary function, e.g., WOPR, GWCT, or WURST.
    ///
    /// \param[in] arg Object for which to retrieve function value, e.g., a
    /// well name.
    ///
    /// \return Current value of summary function for named entity.
    double get(std::string_view func, std::string_view arg) const;

    /// Retrieve function value.
    ///
    /// \param[in] key Combined key for a unique summary vector, e.g.,
    /// WOPR:PROD1, GGOR:FIELD, or SUBUNIT:PROD1:42.
    ///
    /// \return Current value of summary function for named entity.
    double get(const std::string& key) const;

    /// Retrieve name of all wells for which specified summary function is
    /// defined.
    ///
    /// \param[in] func Named well-level summary function, e.g., WOPR or
    /// WMCTL.
    ///
    /// \return All wells for which the named summary function is defined.
    std::vector<std::string> wells(const std::string& func) const;

    /// Get read-only access to run's well lists.
    ///
    /// Convenience method.
    const WListManager& wlist_manager() const
    {
        return this->wListMgr_;
    }

private:
    /// Run's current summary vectors.  Read-only.
    std::reference_wrapper<const SummaryState> summaryState_;

    /// Run's active well lists.
    std::reference_wrapper<const WListManager> wListMgr_;

    /// Read/write container of function values.
    ///
    /// Primary source for get() requests, and only object for which add()
    /// requests are destined.
    std::map<std::string, double> values_{};
};

} // namespace Opm::Action

#endif // ActionContext_HPP
