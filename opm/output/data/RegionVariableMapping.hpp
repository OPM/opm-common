/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media Project (OPM).

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

#ifndef OPM_OUTPUT_DATA_REGIONVARIABLEMAPPING_HPP
#define OPM_OUTPUT_DATA_REGIONVARIABLEMAPPING_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// \file Component for mapping region set names and region variable names
/// to numeric indices.

namespace Opm::data {

    /// Map named region sets and named region variables to numeric indices
    class RegionVariableMapping
    {
    public:
        /// Named region set.
        struct RegionSet
        {
            /// Region set name.
            ///
            /// Common examples include "FIPNUM" and "FIPABC" or similar.
            std::string name{};
        };

        /// Named region variable.
        struct Variable
        {
            /// Region variable name.
            ///
            /// Common examples include "ROPR", "RGIT", and "ROEW".
            std::string name{};
        };

        /// Numeric variable index.
        ///
        /// Provided as an optimisation to bypass the name-to-index lookup
        /// when client code needs to query whether or not a particular
        /// region level variable is a cumulative quantity.
        ///
        /// It is the client code's responsibility to ensure that the
        /// numeric index is correct as no checking will be performed.
        struct VariableIdx
        {
            /// Numeric index of particular region level variable.
            std::size_t idx{};
        };

        /// Prepare internal structures to register name-to-index region set
        /// and variable mappings.
        void prepareRegistration();

        /// Finalise internal name-to-index mapping structures.
        ///
        /// Ensures that all region set and variable names are unique.  If a
        /// name is registered multiple times, then the index corresponding
        /// to that name will be that of the first registration.
        void commitStructure();

        /// Register a named region set.
        ///
        /// Must not be called after commitStructure() and will throw an
        /// exception of type \code std::logic_error \endcode if this
        /// happens.
        ///
        /// \param[in] rset Region set.
        void add(const RegionSet& rset);

        /// Register a named region level variable.
        ///
        /// Must not be called after commitStructure() and will throw an
        /// exception of type \code std::logic_error \endcode if this
        /// happens.
        ///
        /// \param[in] var Region level variable.
        ///
        /// \param[in] is_cumulative Whether or not the variable \p var is a
        /// cumulative quantity.
        void add(const Variable& var, const bool is_cumulative);

        /// Number of unique named region sets known to current mapping.
        ///
        /// Only meaningful after calling member function commitStructure().
        auto numRegionSets() const { return this->regsets_.names().size(); }

        /// Number of unique named region level variables.
        ///
        /// Only meaningful after calling member function commitStructure().
        auto numVariables() const { return this->vars_.names().size(); }

        /// Read-only sequence of named region sets known to current mapping.
        ///
        /// Only meaningful after calling member function commitStructure().
        ///
        /// Region sets presented in alphabetical order.
        decltype(auto) regionSets() const { return this->regsets_.names(); }

        /// Read-only sequence of named region level variables.
        ///
        /// Only meaningful after calling member function commitStructure().
        ///
        /// Region variables presented in alphabetical order.
        decltype(auto) variables() const { return this->vars_.names(); }

        /// Retrieve numeric index of named region set.
        ///
        /// Not meaningful before calling commitStructure() and will throw
        /// an exception of type \code std::logic_error \endcode in that
        /// case.
        ///
        /// \param[in] rset Named region set.
        ///
        /// \return Numeric index of region set \p rset.
        auto index(const RegionSet& rset) const
        {
            this->ensureFinalStructure("region set", rset.name);
            return this->regsets_.index(rset.name);
        }

        /// Retrieve numeric index of named region level variable.
        ///
        /// Not meaningful before calling commitStructure() and will throw
        /// an exception of type \code std::logic_error \endcode in that
        /// case.
        ///
        /// \param[in] var Named region level variable.
        ///
        /// \return Numeric index of region variable \p var.
        auto index(const Variable& var) const
        {
            this->ensureFinalStructure("variable", var.name);
            return this->vars_.index(var.name);
        }

        /// Query whether or not a named region variable represents a
        /// cumulative quantity.
        ///
        /// Not meaningful before calling commitStructure() and will throw
        /// an exception of type \code std::logic_error \endcode in that
        /// case.
        ///
        /// \param[in] var Named region level variable.
        ///
        /// \return Whether or not \p var is a cumulative quantity.
        /// Effectively returns the \c is_cumulative parameter of the add()
        /// member function call that first registered \p var.  Nullopt if
        /// \p var is not a known region level variable.
        std::optional<bool> isCumulative(const Variable& var) const
        {
            const auto i = this->index(var);
            if (! i.has_value()) { return {}; }

            return { this->isCumulative(VariableIdx { *i }) };
        }

        /// Query whether or not a region variable is a cumulative quantity.
        ///
        /// Not meaningful before calling commitStructure().
        ///
        /// \param[in] i Numeric index of specific region variable.  Must be
        /// in the range [0..numVariables()-1], and should typically be the
        /// result of a prior call to member function index() for a
        /// particular region variable name.
        ///
        /// \return Whether or not region variable \p i is a cumulative
        /// quantity.  Effectively returns the \c is_cumulative parameter of
        /// add() member function call that first registered the i-th region
        /// variable in index() order.
        bool isCumulative(const VariableIdx& i) const
        { return this->is_cumulative_[i.idx]; }

    private:
        /// Sorted collection of names
        class NameLookup
        {
        public:
            /// Clear internal data structures.
            void clear() { this->names_.clear(); }

            /// Add a new name to current collection.
            void add(const std::string& name) { this->names_.push_back(name); }

            /// Sort name collection and prune duplicates.
            ///
            /// \return Indices, in sorted order, of original names that are
            /// preserved after sorting and pruning.
            std::vector<std::vector<std::string>::size_type> commit();

            /// Current collection of names.
            ///
            /// Sorted and unique when called *after* commit().
            const std::vector<std::string>& names() const { return this->names_; }

            /// Numeric index of particular name.
            ///
            /// Uses binary search and must therefore not be called prior to
            /// calling commit().
            ///
            /// \return Numeric index of \p name in current collection.
            /// Nullopt if \p name does not exist in the collection.
            std::optional<std::vector<std::string>::size_type>
            index(const std::string& name) const;

        private:
            /// Current collection of names.
            ///
            /// Sorted and unique after client code calls commit().
            std::vector<std::string> names_{};
        };

        /// Collection of named region sets.
        NameLookup regsets_{};

        /// Collection of named region level variables.
        NameLookup vars_{};

        /// Flags for whether or not a particular region level variable is a
        /// cumulative quantity.
        std::vector<bool> is_cumulative_{};

        /// Whether or not client code has called commitStructure().
        bool is_final_{false};

        /// Ensure that registering a new name is possible.
        ///
        /// Inspects the \c is_final_ flag and throws an exception of type
        /// \code std::logic_error \endcode if is_final_ is \c true.
        ///
        /// \param[in] type Name type.  Typically "region set" or
        /// "variable".  Used for diagnostic message if is_final_ is \c
        /// true.
        ///
        /// \param[in] name Object name.  Used in diagnostic message if
        /// is_final_ is \c true.
        void ensureRegistrationPossible(std::string_view type,
                                        std::string_view name) const;

        /// Ensure that name lookup structure is complete.
        ///
        /// Inspects the \c is_final_ flag and throws an exception of type
        /// \code std::logic_error \endcode if is_final_ is \c false.
        ///
        /// \param[in] type Name type.  Typically "region set" or
        /// "variable".  Used for diagnostic message if is_final_ is \c
        /// false.
        ///
        /// \param[in] name Object name.  Used in diagnostic message if
        /// is_final_ is \c false.
        void ensureFinalStructure(std::string_view type,
                                  std::string_view name) const;

        /// Reorder and compress variables' cumulative flags.
        ///
        /// This is one of the last steps of commitStructure().
        ///
        /// \param[in] i Sorted and unique original indices of those region
        /// level variables that are preserved after finalising the
        /// name-lookup structure.  Expected to be the return value from
        /// \code NameLookup::commit() \endcode.
        void makeUniqueCumulative(const std::vector<std::vector<std::string>::size_type>& i);
    };

} // namespace Opm::data

#endif // OPM_OUTPUT_DATA_REGIONVARIABLEMAPPING_HPP
