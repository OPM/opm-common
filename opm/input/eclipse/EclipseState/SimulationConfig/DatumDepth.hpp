// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2024 Equinor ASA.

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

#ifndef DATUM_DEPTH_HPP_INCLUDED
#define DATUM_DEPTH_HPP_INCLUDED

#include <cassert>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

/// Encapsulation of the DATUM* familiy of SOLUTION section keywords which
/// specify depths against which to compute depth corrected cell (block),
/// region, and field-level pressures/potentials.  This information goes
/// into the definition of summary keywords [FRB]PP[OGW].

namespace Opm {
    class SOLUTIONSection;
} // namespace Opm

namespace Opm {

    /// Wrapper class which handles the full familiy of datum depth
    /// keywords.
    class DatumDepth
    {
    public:
        /// Default constructor.
        DatumDepth() = default;

        /// Constructor from SOLUTION section information
        ///
        /// \param[in] soln SOLUTION section container.
        explicit DatumDepth(const SOLUTIONSection& soln);

        /// Form serialisation test object for the case of no datum depth
        /// (=> datum depth = zero).
        static DatumDepth serializationTestObjectZero();

        /// Form serialisation test object for the case of a single global
        /// reference depth--either from the DATUM keyword or from the datum
        /// depth of the first equilibration region.
        static DatumDepth serializationTestObjectGlobal();

        /// Form serialisation test object for the case of per-region
        /// reference depths in the DATUMR keyword.  Tied to the standard
        /// FIPNUM region set.
        static DatumDepth serializationTestObjectDefaultRegion();

        /// Form serialisation test object for the case of per-region
        /// reference depth for user-defined region sets in the DATUMRX
        /// keyword.
        static DatumDepth serializationTestObjectUserDefined();

        /// Retrieve datum depth in particular region of standard FIPNUM
        /// region set.
        ///
        /// \param[in] region Zero-based region index.
        ///
        /// \return Datum/reference depth in region \p region of standard
        ///   FIPNUM region set.
        double operator()(const int region) const
        {
            return (*this)("FIPNUM", region);
        }

        /// Retrieve datum depth in particular region of named region set.
        ///
        /// \param[in] rset Region set name, e.g., FIPNUM.
        ///
        /// \param[in] region Zero-based region index.
        ///
        /// \return Datum/reference depth in region \p region of region set
        /// \p rset.
        double operator()(std::string_view rset, const int region) const
        {
            return std::visit([rset, region](const auto& datumDepthImpl)
            { return datumDepthImpl(rset, region); }, this->datum_);
        }

        /// Equality predicate.
        ///
        /// \param[in] that Object against which \c *this will be compared
        /// for equality.
        ///
        /// \return Whether or not \c *this equals \p that.
        bool operator==(const DatumDepth& that) const
        {
            return this->datum_ == that.datum_;
        }

        /// Serialisation interface.
        ///
        /// \tparam Serializer Object serialisation protocol implementation
        ///
        /// \param[in,out] serializer Serialisation object
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->datum_);
        }

    private:
        /// Neither DATUM* nor EQUIL specified => datum depth = 0
        class Zero
        {
        public:
            /// Retrieve datum depth.
            ///
            /// Always returns zero.
            ///
            /// \param[in] rset Region set name, e.g., FIPNUM.
            ///
            /// \param[in] region Zero-based region index.
            ///
            /// \return Datum/reference depth in region \p region of region
            /// set \p rset (= 0).
            double operator()([[maybe_unused]] std::string_view rset,
                              [[maybe_unused]] const int        region) const
            {
                return 0.0;
            }

            /// Form serialisation test object.
            static Zero serializationTestObject() { return {}; }

            /// Equality predicate.
            bool operator==(const Zero&) const { return true; }

            /// Serialisation interface
            ///
            /// \tparam Serializer Object serialisation protocol
            /// implementation
            ///
            /// \param[in,out] serializer Serialisation object
            template <class Serializer>
            void serializeOp([[maybe_unused]] Serializer& serializer)
            {}                  // Nothing to do
        };

        /// Implementation of DATUM keyword with fallback to EQUIL
        class Global
        {
        public:
            /// Default constructor.
            Global() = default;

            /// Constructor from SOLUTION section information
            ///
            /// \param[in] soln SOLUTION section container.
            explicit Global(const SOLUTIONSection& soln);

            /// Retrieve datum depth.
            ///
            /// \param[in] rset Region set name, e.g., FIPNUM.
            ///
            /// \param[in] region Zero-based region index.
            ///
            /// \return Datum/reference depth in region \p region of region
            /// set \p rset (= globally configured reference depth).
            double operator()([[maybe_unused]] std::string_view rset,
                              [[maybe_unused]] const int        region) const
            {
                return this->depth_;
            }

            /// Form serialisation test object.
            static Global serializationTestObject();

            /// Equality predicate.
            ///
            /// \param[in] that Object against which \c *this will be
            /// compared for equality.
            ///
            /// \return Whether or not \c *this equals \p that.
            bool operator==(const Global& that) const
            {
                return this->depth_ == that.depth_;
            }

            /// Serialisation interface
            ///
            /// \tparam Serializer Object serialisation protocol
            /// implementation
            ///
            /// \param[in,out] serializer Serialisation object
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->depth_);
            }

        private:
            /// Globally configured reference depth for all regions in all
            /// region sets.
            double depth_{};
        };

        /// Implementation of DATUMR keyword
        class DefaultRegion
        {
        public:
            /// Default constructor.
            DefaultRegion() = default;

            /// Constructor from SOLUTION section information
            ///
            /// \param[in] soln SOLUTION section container.
            explicit DefaultRegion(const SOLUTIONSection& soln);

            /// Form serialisation test object.
            static DefaultRegion serializationTestObject();

            /// Retrieve datum depth.
            ///
            /// \param[in] rset Region set name, e.g., FIPNUM.
            ///
            /// \param[in] region Zero-based region index.
            ///
            /// \return Datum/reference depth in region \p region of region
            /// set \p rset.
            double operator()([[maybe_unused]] std::string_view rset,
                              const int region) const
            {
                assert (! this->depth_.empty());

                // Note: depth_.back() is intentional for region >= size().
                // If the input supplies fewer depth values than there are
                // regions, then the remaining regions implicitly have the
                // same datum depth as the last fully specified region.
                return (region < static_cast<int>(this->depth_.size()))
                    ? this->depth_[region] : this->depth_.back();
            }

            /// Equality predicate.
            ///
            /// \param[in] that Object against which \c *this will be
            /// compared for equality.
            ///
            /// \return Whether or not \c *this equals \p that.
            bool operator==(const DefaultRegion& that) const
            {
                return this->depth_ == that.depth_;
            }

            /// Serialisation interface
            ///
            /// \tparam Serializer Object serialisation protocol
            /// implementation
            ///
            /// \param[in,out] serializer Serialisation object
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->depth_);
            }

        private:
            /// Reference depths for all regions in default (FIPNUM) region
            /// set.
            std::vector<double> depth_{};
        };

        /// Implementation of DATUMRX keyword
        class UserDefined
        {
        public:
            /// Default constructor.
            UserDefined() = default;

            /// Constructor from SOLUTION section information
            ///
            /// \param[in] soln SOLUTION section container.
            explicit UserDefined(const SOLUTIONSection& soln);

            /// Form serialisation test object.
            static UserDefined serializationTestObject();

            /// Retrieve datum depth.
            ///
            /// \param[in] rset Region set name, e.g., FIPNUM.
            ///
            /// \param[in] region Zero-based region index.
            ///
            /// \return Datum/reference depth in region \p region of region
            /// set \p rset (= globally configured reference depth).
            double operator()(std::string_view rset, const int region) const;

            /// Equality predicate.
            ///
            /// \param[in] that Object against which \c *this will be
            /// compared for equality.
            ///
            /// \return Whether or not \c *this equals \p that.
            bool operator==(const UserDefined& that) const
            {
                return (this->rsetNames_ == that.rsetNames_)
                    && (this->rsetStart_ == that.rsetStart_)
                    && (this->depth_     == that.depth_)
                    && (this->default_   == that.default_)
                    && (this->rsetIndex_ == that.rsetIndex_)
                    ;
            }

            /// Serialisation interface
            ///
            /// \tparam Serializer Object serialisation protocol
            /// implementation
            ///
            /// \param[in,out] serializer Serialisation object
            template <class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->rsetNames_);
                serializer(this->rsetStart_);
                serializer(this->depth_);
                serializer(this->default_);
                serializer(this->rsetIndex_);
            }

        private:
            /// Known region sets, in order of appearance in DATUMRX.
            /// Initial 'FIP' name prefix pruned.
            std::vector<std::string> rsetNames_{};

            /// Region set start pointers into \c depth_
            std::vector<std::vector<double>::size_type> rsetStart_{};

            /// Per region region reference depth values indexed by \c
            /// rsetStart_ and the region index.
            std::vector<double> depth_{};

            /// Per-region reference depth of fallback specification
            /// (DATUMR) if present.
            std::vector<double> default_{};

            /// Ordered indices into \c rsetNames_ and \c rsetStart_ to
            /// enable O(log n) binary search lookup of name->index mapping.
            std::vector<std::vector<std::string>::size_type> rsetIndex_{};

            /// Retrive datum depth for specific region in specific region set.
            ///
            /// \param[in] rset Region set name.  For diagnostic purposes only.
            ///
            /// \param[in] begin Start of region set's subrange in \c depth_
            ///   or \code default_.begin() if \p rset is not among the
            ///   known region sets.
            ///
            /// \param[in] end One past the end of region set's subrange in
            ///   \c depth_ or \code default_.end() if \p rset is not among
            ///   the known region sets.
            ///
            /// \param[in] ix Zero-based region index.
            ///
            /// \return Datum depth for region \p ix in region set \p rset
            ///   if range [begin, end) is non-empty.  Exception of type \c
            ///   invalid_argument otherwise (end == begin).
            double depthValue(std::string_view                     rset,
                              std::vector<double>::const_iterator  begin,
                              std::vector<double>::const_iterator  end,
                              std::vector<double>::difference_type ix) const;
        };

        /// Datum depth implementation object.
        std::variant<Zero, Global, DefaultRegion, UserDefined> datum_{};
    };

} // namespace Opm

#endif // DATUM_DEPTH_HPP_INCLUDED
