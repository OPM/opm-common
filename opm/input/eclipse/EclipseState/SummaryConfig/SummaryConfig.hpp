/*
  Copyright 2015 Statoil ASA.

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

#ifndef OPM_SUMMARY_CONFIG_HPP
#define OPM_SUMMARY_CONFIG_HPP

#include <opm/io/eclipse/SummaryNode.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace Opm {
    class AquiferConfig;
    class Deck;
    class EclipseState;
    class ErrorGuard;
    class FieldPropsManager;
    class GridDims;
    class ParseContext;
    class Schedule;
} // namespace Opm

namespace Opm {

    /// Definition of a single summary vector.
    ///
    /// Collects the vector name (summary keyword), the vector entity (e.g.,
    /// a well or group name), the vector "number" (e.g., a cell or segment
    /// index), and any applicable region names (for region level vectors).
    class SummaryConfigNode
    {
    public:
        /// Summary vector level (field, well, region, &c).
        using Category = Opm::EclIO::SummaryNode::Category;

        /// Summary vector type (rates, cumulative, pressure, &c)
        using Type = Opm::EclIO::SummaryNode::Type;

        /// Default constructor.
        ///
        /// Resulting object is mostly usable as the target of a
        /// deserialisation operation.
        SummaryConfigNode() = default;

        /// Constructor
        ///
        /// \param[in] keyword Summary vector name.
        ///
        /// \param[in] cat Summary vector level.
        ///
        /// \param[in] loc_arg Keyword location.  Mostly for diagnostic
        /// purposes.
        explicit SummaryConfigNode(std::string keyword,
                                   const Category cat,
                                   KeywordLocation loc_arg);

        /// Create a serialisation test object.
        static SummaryConfigNode serializationTestObject();

        /// Assign vector type.
        ///
        /// \param[in] type Summary vector type (e.g., rate, cumulative,
        /// ratio, pressure).
        ///
        /// \return \code *this \endcode
        SummaryConfigNode& parameterType(const Type type);

        /// Assign vector's named entity.
        ///
        /// \param[in] name Summary vector's named entity such as a well or
        /// group name.
        ///
        /// \return \code *this \endcode
        SummaryConfigNode& namedEntity(std::string name);

        /// Assign vector's numeric ID.
        ///
        /// \param[in] num Summary vector's "number" such as a
        /// cell/connection index, a completion number or a segment number.
        ///
        /// \return \code *this \endcode
        SummaryConfigNode& number(const int num);

        /// Assign vector's UDQ flag.
        ///
        /// \param[in] userDefined Whether or not vector is a user defined
        /// quantity.
        ///
        /// \return \code *this \endcode
        SummaryConfigNode& isUserDefined(const bool userDefined);

        /// Assign vector's associated region name.
        ///
        /// \param[in] fip_region Summary vector's associated region.
        ///
        /// \return \code *this \endcode
        SummaryConfigNode& fip_region(const std::string& fip_region);

        /// Retrieve summary vector name.
        const std::string& keyword() const { return this->keyword_; }

        /// Retrieve summary vector's level.
        Category category() const { return this->category_; }

        /// Retrieve summary vector's type.
        Type type() const { return this->type_; }

        /// Retrieve summary vector's named entity.
        const std::string& namedEntity() const { return this->name_; }

        /// Retrieve summary vector's associated numeric ID.
        int number() const { return this->number_; }

        /// Retrieve summary vector's UDQ flag.
        bool isUserDefined() const { return this->userDefined_; }

        /// Retrieve summary vector's associated region.
        const std::string& fip_region() const { return *this->fip_region_ ; }

        /// Retrieve a unique distinguishing identifier for this summary vector.
        std::string uniqueNodeKey() const;

        /// Retrieve summary keyword location in input.
        ///
        /// Mostly provided for diagnostic purposes.
        const KeywordLocation& location() const { return this->loc; }

        /// Convert summary vector definition to low-level SummaryNode object.
        operator EclIO::SummaryNode() const
        {
            return {
                /* keyword = */    this->keyword_,
                /* category = */   this->category_,
                /* type = */       this->type_,
                /* wgname = */     this->name_,
                /* number = */     this->number_,
                /* fip_region = */ this->fip_region_,
                /* lgr = */        {} // std::optional<>
            };
        }

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(keyword_);
            serializer(category_);
            serializer(loc);
            serializer(type_);
            serializer(name_);
            serializer(number_);
            serializer(fip_region_);
            serializer(userDefined_);
        }

    private:
        /// Summary vector name.
        std::string keyword_{};

        /// Summary vector level (e.g., field, group, well, segment, region).
        Category category_{};

        /// Summary vector's type (e.g., rate, cumulative, ratio, pressure).
        Type type_{ Type::Undefined };

        /// Summary vector's named entity (e.g., well or group name).
        std::string name_{};

        /// Summary vector's associated numeric ID.
        ///
        /// Common examples are cell/connection indices or
        /// completion/segment/region numbers.
        int number_{std::numeric_limits<int>::min()};

        /// Summary vector's associated region (for region level vectors).
        std::optional<std::string> fip_region_{};

        /// Whether or not this vector is a user-defined quantity.
        bool userDefined_{false};

        /// Summary keyword's location in input file.
        ///
        /// Mostly for diagnostic purposes.
        KeywordLocation loc{};
    };

    /// Infer summary vector level from keyword name.
    ///
    /// \param[in] keyword Summary vector name.
    ///
    /// \return Summary vector level (e.g., field, group, well, connection,
    /// cell, region, segment) for \p keyword.
    SummaryConfigNode::Category parseKeywordCategory(const std::string& keyword);

    /// Infer summary vector type from keyword name.
    ///
    /// \param[in] keyword Summary vector name.
    ///
    /// \return Summary vector type (e.g., rate, cumulative, ratio,
    /// pressure) for \p keyword.
    SummaryConfigNode::Type parseKeywordType(std::string keyword);

    /// Equality predicate for SummaryConfigNode objects.
    ///
    /// \param[in] lhs Left-hand side object.
    ///
    /// \param[in] rhs Right-hand side object.
    ///
    /// \return Whether or not \p lhs is the same as (equal to) \p rhs.
    bool operator==(const SummaryConfigNode& lhs, const SummaryConfigNode& rhs);

    /// Canonical comparison operator for SummaryConfigNode objects.
    ///
    /// \param[in] lhs Left-hand side object.
    ///
    /// \param[in] rhs Right-hand side object.
    ///
    /// \return Whether or not \p lhs is less than \p rhs.
    bool operator<(const SummaryConfigNode& lhs, const SummaryConfigNode& rhs);

    /// Inequality operator for SummaryConfigNode objects
    ///
    /// \param[in] lhs Left-hand side object.
    ///
    /// \param[in] rhs Right-hand side object.
    ///
    /// \return Whether or not \p lhs is different from \p rhs.
    inline bool operator!=(const SummaryConfigNode& lhs, const SummaryConfigNode& rhs)
    {
        return ! (lhs == rhs);
    }

    /// Less-than-or-equal comparison operator for SummaryConfigNode objects.
    ///
    /// \param[in] lhs Left-hand side object.
    ///
    /// \param[in] rhs Right-hand side object.
    ///
    /// \return Whether or not \p lhs is less than or equal to \p rhs.
    inline bool operator<=(const SummaryConfigNode& lhs, const SummaryConfigNode& rhs)
    {
        return ! (rhs < lhs);
    }

    /// Greater-than comparison operator for SummaryConfigNode objects.
    ///
    /// \param[in] lhs Left-hand side object.
    ///
    /// \param[in] rhs Right-hand side object.
    ///
    /// \return Whether or not \p lhs is greater than \p rhs.
    inline bool operator>(const SummaryConfigNode& lhs, const SummaryConfigNode& rhs)
    {
        return rhs < lhs;
    }

    /// Greater-than-or-equal comparison operator for SummaryConfigNode objects.
    ///
    /// \param[in] lhs Left-hand side object.
    ///
    /// \param[in] rhs Right-hand side object.
    ///
    /// \return Whether or not \p lhs is greater than or equal to \p rhs.
    inline bool operator>=(const SummaryConfigNode& lhs, const SummaryConfigNode& rhs)
    {
        return ! (lhs < rhs);
    }

    /// Collection of run's summary vectors.
    class SummaryConfig
    {
    public:
        /// Convenience type alias for a linear sequence of summary vector
        /// definitions.
        using keyword_list = std::vector<SummaryConfigNode>;

        /// Default constructor.
        SummaryConfig() = default;

        /// Constructor.
        ///
        /// Parses the SUMMARY section of the run's model description and
        /// configures the initial collection of summary vectors.  Parse
        /// failures such as missing or incorrect well names are reported
        /// through the normal ErrorGuard mechanism.
        ///
        /// \param[in] deck Run's model description.
        ///
        /// \param[in] schedule Run's collection of dynamic objects (wells,
        /// groups, &c).  Needed to configure vectors for keywords that
        /// apply to "all" entities such as
        ///
        ///    WOPR
        ///    /
        ///
        /// \param[in] field_props Run's static property container.  Needed
        /// to define region-level vectors, especially for those that use
        /// user-defined region sets such as FIPABC.
        ///
        /// \param[in] aquiferConfig Run's collection of analytic and
        /// numerical aquifers.  Needed to define aquifer level vectors that
        /// apply to "all" aquifers.
        ///
        /// \param[in] parseContext Error handling controls.
        ///
        /// \param[in,out] errors Collection of parse errors encountered
        /// thus far.  Behaviour controlled by \p parseContext.
        SummaryConfig(const Deck&              deck,
                      const Schedule&          schedule,
                      const FieldPropsManager& field_props,
                      const AquiferConfig&     aquiferConfig,
                      const ParseContext&      parseContext,
                      ErrorGuard&              errors);

        /// Constructor.
        ///
        /// Trampoline for expiring ErrorGuard objects.  This constructor
        /// should arguably not exist.
        ///
        /// Parses the SUMMARY section of the run's model description and
        /// configures the initial collection of summary vectors.  Parse
        /// failures such as missing or incorrect well names are reported
        /// through the normal ErrorGuard mechanism, though the ErrorGuard
        /// object does not exist when the constructor completes.
        ///
        /// \tparam T Error guard type.  Must be ErrorGuard.
        ///
        /// \param[in] deck Run's model description.
        ///
        /// \param[in] schedule Run's collection of dynamic objects (wells,
        /// groups, &c).  Needed to configure vectors for keywords that
        /// apply to "all" entities such as
        ///
        ///    WOPR
        ///    /
        ///
        /// \param[in] field_props Run's static property container.  Needed
        /// to define region-level vectors, especially for those that use
        /// user-defined region sets such as FIPABC.
        ///
        /// \param[in] aquiferConfig Run's collection of analytic and
        /// numerical aquifers.  Needed to define aquifer level vectors that
        /// apply to "all" aquifers.
        ///
        /// \param[in] parseContext Error handling controls.
        ///
        /// \param[in,out] errors Collection of parse errors encountered
        /// thus far.  Behaviour controlled by \p parseContext.
        template <typename T>
        SummaryConfig(const Deck&              deck,
                      const Schedule&          schedule,
                      const FieldPropsManager& field_props,
                      const AquiferConfig&     aquiferConfig,
                      const ParseContext&      parseContext,
                      T&&                      errors);

        /// Constructor.
        ///
        /// Parses the SUMMARY section of the run's model description and
        /// configures the initial collection of summary vectors.  Parse
        /// failures such as missing or incorrect well names generate
        /// exceptions and stop the simulation run.
        ///
        /// \param[in] deck Run's model description.
        ///
        /// \param[in] schedule Run's collection of dynamic objects (wells,
        /// groups, &c).  Needed to configure vectors for keywords that
        /// apply to "all" entities such as
        ///
        ///    WOPR
        ///    /
        ///
        /// \param[in] field_props Run's static property container.  Needed
        /// to define region-level vectors, especially for those that use
        /// user-defined region sets such as FIPABC.
        ///
        /// \param[in] aquiferConfig Run's collection of analytic and
        /// numerical aquifers.  Needed to define aquifer level vectors that
        /// apply to "all" aquifers.
        SummaryConfig(const Deck&              deck,
                      const Schedule&          schedule,
                      const FieldPropsManager& field_props,
                      const AquiferConfig&     aquiferConfig);

        /// Constructor.
        ///
        /// Forms collection from existing set of summary vector
        /// definitions.
        ///
        /// \param[in] keywords Summary vector definitions.
        ///
        /// \param[in] shortKwds Unique vector names in \p keywords.
        ///
        /// \param[in] smryKwds Unique vector keys in \p keywords.
        SummaryConfig(const keyword_list&          keywords,
                      const std::set<std::string>& shortKwds,
                      const std::set<std::string>& smryKwds);

        /// Create a serialisation test object.
        static SummaryConfig serializationTestObject();

        /// Beginning of sequence of summary vector definitions.
        ///
        /// Exists to support using standard algorithms and range-for.
        auto begin() const { return this->m_keywords.begin(); }

        /// One past the end of the sequence of summary vector definitions.
        ///
        /// Exists to support using standard algorithms and range-for.
        auto end() const { return this->m_keywords.end(); }

        /// Number of summary vectors in current collection.
        std::size_t size() const { return this->m_keywords.size(); }

        /// Partially defined summary vectors relating to fracturing processes.
        const keyword_list& extraFracturingVectors() const
        {
            return this->extraFracturingVectors_;
        }

        /// Incorporate other vector collection into current.
        ///
        /// \param[in] other Collection of vector definitions.
        ///
        /// \return \code *this \endcode
        SummaryConfig& merge(const SummaryConfig& other);

        /// Incorporate other vector collection into current.
        ///
        /// Assumes ownership over the vector definitions in the other
        /// collection.
        ///
        /// \param[in,out] other Collection of vector definitions.  Will be
        /// empty on return.
        ///
        /// \return \code *this \endcode
        SummaryConfig& merge(SummaryConfig&& other);

        /// Form definitions from vectors used in UDQs and ACTIONX.
        ///
        /// \param[in] extraKeys Vector names used in defining expressions
        /// for UDQs or in ACTIONX condition blocks.
        ///
        /// \param[in] es Run's static objects such as property arrays,
        /// region definitions, and aquifers.  Note that this function will
        /// create vector definitions for all applicable entities.  In other
        /// words, we will create one vector for each well when processing a
        /// well level vector name and similarly for groups, connections,
        /// segments, and regions.  Moreover, for the purpose of this
        /// function, FIELD is treated as a regular group name.  This
        /// treatment enables using expressions such as "GOPR FIELD" in an
        /// ACTIONX condition block.
        ///
        /// \param[in] sched Run's dynamic objects such as wells and groups.
        ///
        /// \return Vector definitions for the vectors in \p extraKeys that
        /// did not already exist.
        keyword_list
        registerRequisiteUDQorActionSummaryKeys(const std::vector<std::string>& extraKeys,
                                                const EclipseState&             es,
                                                const Schedule&                 sched);

        /// Query existence of summary vector name.
        ///
        /// \param[in] keyword Summary vector name.  Should be a regular
        /// summary keyword like "WWCT" or "FOPR" rather than a fully
        /// qualified vector key.
        ///
        /// \return Whether or not \p keyword exists in the current
        /// collection.
        bool hasKeyword(const std::string& keyword) const;

        /// Query existence of summary vector name with pattern matching.
        ///
        /// \param[in] keywordPattern Shell-style pattern for summary vector
        /// names.  Should be a regular summary keyword names like "WWCT" or
        /// "FOPR" rather than a fully qualified vector key.  As an example,
        /// \code match("W*") \endcode will query whether or not any
        /// well-level summary vectors have been defined.
        ///
        /// \return Whether or not any vectors matching the \p
        /// keywordPattern exist in the current collection.
        bool match(const std::string& keywordPattern) const;

        /// Retrieve all vector definitions matching a vector name pattern.
        ///
        /// \param[in] keywordPattern Shell-style pattern for summary vector
        /// names.  Should be a regular summary keyword names like "WWCT" or
        /// "FOPR" rather than a fully qualified vector key.  As an example,
        /// \code keywords("ROFT*") \endcode will retrieve all existing
        /// vector definitions for all inter-region cumulative oil flow.
        ///
        /// \return List of definitions mathcing the \p keywordPattern.
        keyword_list keywords(const std::string& keywordPattern) const;

        /// Query existence of fully qualified summary vector key.
        ///
        /// \param[in] key Fully qualified summary key like "SOFR:P-123:42".
        ///
        /// \return Whether or not the summary \p key exists in the current
        /// collection.
        bool hasSummaryKey(const std::string& key) const;

        /// Query whether or not a 3D dynamic property is needed to evaluate
        /// some/all summary vectors.
        ///
        /// \param[in] keyword Name of 3D dynamic property (e.g., "PRESSURE"
        /// or "OIP").
        ///
        /// \return Whether or not the \p keyword property is needed to
        /// evaluate some or all of the summary vectors in this collection.
        bool require3DField(const std::string& keyword) const;

        /// Named region sets needed across all known region level vectors.
        std::set<std::string> fip_regions() const;

        /// Named region sets needed across all known inter-region vectors.
        std::set<std::string> fip_regions_interreg_flow() const;

        /// Equality predicate.
        ///
        /// \param[in] data Object against which \code *this \endcode will
        /// be tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p data.
        bool operator==(const SummaryConfig& data) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_keywords);
            serializer(short_keywords);
            serializer(summary_keywords);
        }

        /// Whether or not to create a human-readable .RSM file at the end
        /// of the simulation run.
        bool createRunSummary() const
        { return runSummaryConfig.create; }

        /// Retrieve summary vector definition from linear index
        ///
        /// \param[in] index Linear index in the range [0..size()).
        ///
        /// \return Vector definition at position \p index in the internal
        /// collection.
        const SummaryConfigNode& operator[](std::size_t index) const;

    private:
        /// Primary constructor.
        ///
        /// Final delegate from constructor chain.
        ///
        /// Parses the SUMMARY section of the run's model description and
        /// configures the initial collection of summary vectors.  Parse
        /// failures such as missing or incorrect well names are reported
        /// through the normal ErrorGuard mechanism.
        ///
        /// \param[in] deck Run's model description.
        ///
        /// \param[in] schedule Run's collection of dynamic objects (wells,
        /// groups, &c).  Needed to configure vectors for keywords that
        /// apply to "all" entities such as
        ///
        ///    WOPR
        ///    /
        ///
        /// \param[in] field_props Run's static property container.  Needed
        /// to define region-level vectors, especially for those that use
        /// user-defined region sets such as FIPABC.
        ///
        /// \param[in] aquiferConfig Run's collection of analytic and
        /// numerical aquifers.  Needed to define aquifer level vectors that
        /// apply to "all" aquifers.
        ///
        /// \param[in] parseContext Error handling controls.
        ///
        /// \param[in,out] errors Collection of parse errors encountered
        /// thus far.  Behaviour controlled by \p parseContext.
        ///
        /// \param[in] dims Model dimensions.  Used to translate IJK cell
        /// index triplets to linear Cartesian indices and for diagnostic
        /// purposes.
        SummaryConfig(const Deck&              deck,
                      const Schedule&          schedule,
                      const FieldPropsManager& field_props,
                      const AquiferConfig&     aquiferConfig,
                      const ParseContext&      parseContext,
                      ErrorGuard&              errors,
                      const GridDims&          dims);

        /// Run's configured summary vectors.
        keyword_list m_keywords{};

        /// Additional and incomplete summary vectors defined for run-time
        /// allocation in runs featuring fracturing processes.
        keyword_list extraFracturingVectors_{};

        /// Summary vector names.
        ///
        /// Acceleration structure for vector name existence queries.
        /// Contains only the vector names such as FOPR, WWCT, SOFR, or
        /// COPT, without any assocated named entities or numeric IDs.
        std::set<std::string> short_keywords{};

        /// Unique keys for all vectors in current collection.
        std::set<std::string> summary_keywords{};

        /// Configuration for run's .RSM file output.
        struct {
            /// Whether or not to create a human-readable .RSM file at the
            /// end of the simulation run.
            bool create { false };

            /// Whether or not to output the .RSM file in "narrow" format.
            ///
            /// Ignored.
            bool narrow { false };

            /// Whether or not to create a separate .RSM file instead of
            /// placing the run summary at the end of the .PRT file.
            ///
            /// Ignored.
            bool separate { true };
        } runSummaryConfig{};

        /// Configure run's .RSM file output.
        ///
        /// This function assigns specific members of \c runSummaryConfig.
        ///
        /// \param[in] keyword Run summary directive.  Expected to be one of
        /// "RUNSUM", "NARROW", or "SEPARATE" which affect the 'create',
        /// 'narrow', and 'separate' members of \c runSummaryConfig
        /// respectively.
        void handleProcessingInstruction(const std::string& keyword);
    };

} // namespace Opm

#endif // OPM_SUMMARY_CONFIG_HPP
