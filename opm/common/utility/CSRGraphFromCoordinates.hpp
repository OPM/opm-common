/*
  Copyright 2016 SINTEF ICT, Applied Mathematics.
  Copyright 2016 Statoil ASA.
  Copyright 2022 Equinor ASA

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

#ifndef OPM_UTILITY_CSRGRAPHFROMCOORDINATES_HPP
#define OPM_UTILITY_CSRGRAPHFROMCOORDINATES_HPP

#include <cstddef>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
/// \file
///
/// Facility for converting collection of region ID pairs into a sparse
/// (CSR) adjacency matrix representation of a graph.  Supports O(nnz)
/// compression.

namespace Opm { namespace utility {

    /// Form CSR adjacency matrix representation of unstructured graphs.
    /// Optionally maps vertex pairs to compressed indices to enable O(1)
    /// per-element lookup in assembly-like operations.
    ///
    /// \tparam VertexID ID type of an abstract vertex in the graph.  Could
    ///    for instance be used to represent a cell index or a region index.
    ///    Must be an integral type.
    ///
    /// \tparam TrackCompressedIdx Whether or not to form a mapping relation
    ///    for vertex pairs to compressed indices.  Default value, false,
    ///    bypasses this mapping relation and conserves memory.
    ///
    /// \tparam PermitSelfConnections Whether or not to allow connections of
    ///    the form i->i--i.e., diagonal elements.  Default value, \c false,
    ///    does not generate connections from a vertex to itself.
    template <typename VertexID = int, bool TrackCompressedIdx = false, bool PermitSelfConnections = false>
    class CSRGraphFromCoordinates
    {
    private:
        using BaseVertexID = std::remove_cv_t<VertexID>;

        static_assert(std::is_integral_v<BaseVertexID>,
                      "The VertexID must be an integral type");

    public:
        /// Representation of neighbouring regions.
        using Neighbours = std::vector<BaseVertexID>;

        /// Offset into neighbour array.
        using Offset = typename Neighbours::size_type;

        /// CSR start pointers.
        using Start = std::vector<Offset>;

        /// Clear all internal buffers, but preserve allocated capacity.
        void clear();

        /// Add flow rate connection between regions.
        ///
        /// \param[in] v1 First vertex in vertex pair.  Used as row index.
        ///
        /// \param[in] v2 Second vertex in vertex pair.  Used as column index.
        ///
        /// If both vertex IDs are the same, and class template argument \c
        /// PermitSelfConnections is in its default state of \c false, then
        /// this function does nothing.
        void addConnection(VertexID v1, VertexID v2);

        /// Apply vertex merges to all vertex groups
        Offset applyVertexMerges();


        /// Form CSR adjacency matrix representation of input graph from
        /// connections established in previous calls to addConnection().
        ///
        /// \param[in] maxNumVertices Number of rows in resulting CSR
        ///     matrix.  If prior calls to addConnection() supply vertex IDs
        ///     (row indices) greater than or equal to \p maxNumVertices,
        ///     then method compress() will throw \code
        ///     std::invalid_argument \endcode.
        ///
        /// \param[in] expandExistingIdxMap Whether or not preserve and
        ///     update the existing compressed index map.  This is
        ///     potentially useful for the case of adding new connections to
        ///     an already compressed graph.  The default setting, \c false,
        ///     will disregard any existing index map and create the index
        ///     map from scratch.  This runtime parameter is unused if the
        ///     class is not configured to track compressed indices through
        ///     the TrackCompressedIdx class parameter.
        void compress(Offset maxNumVertices,
                      bool expandExistingIdxMap = false);

        /// Retrieve number of rows (source entities) in input graph.
        /// Corresponds to value of argument passed to compress().  Valid
        /// only after calling compress().
        Offset numVertices() const;

        /// Retrieve number of edges (non-zero matrix elements) in input
        /// graph.
        Offset numEdges() const;

        /// Read-only access to compressed structure's start pointers.
        const Start& startPointers() const
        {
            return this->csr_.startPointers();
        }

        /// Read-only access to compressed structure's column indices,
        /// ascendingly sorted per row.
        const Neighbours& columnIndices() const
        {
            return this->csr_.columnIndices();
        }

        /// Read-only access to mapping from input order vertex pairs to
        /// compressed structure's edge index (location in ja/sa).
        ///
        /// Available only if client code sets TrackCompressedIdx=true.
        /// Compiler diagnostic, typically referring to 'enable_if', if
        /// client code tries to call this function without setting
        /// class parameter TrackCompressedIdx=true.
        template <typename Ret = const Start&>
        std::enable_if_t<TrackCompressedIdx, Ret> compressedIndexMap() const
        {
            return this->csr_.compressedIndexMap();
        }

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            this->csr_.write(buffer);
        }

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            auto other = CSR{};
            other.read(buffer);

            this->uncompressed_
                .add(other.maxRowID(),
                     other.maxColID(),
                     other.coordinateFormatRowIndices(),
                     other.columnIndices());
        }

        /// Add a group of vertices that should be merged together.
        /// Must be called before compress().
        ///
        /// \param[in] vertices Vector of vertex IDs to merge
        void addVertexGroup(const std::vector<VertexID>& vertices);

        /// Get the final vertex ID after all merges and renumbering for a given original vertex ID.
        /// Returns the original ID if no merging or renumbering has been done.
        ///
        /// \param[in] originalVertexID The original vertex ID to look up
        /// \return The final vertex ID after merges and renumbering
        VertexID getFinalVertexID(VertexID originalVertexID) const;

    private:
        /// Coordinate format representation of individual contributions to
        /// inter-region flows.
        class Connections
        {
        public:
            /// Add contributions from a single inter-region connection.
            ///
            /// \param[in] r1 Source region.  Zero-based region index/ID.
            ///
            /// \param[in] r2 Destination region.  Zero-based region index.
            void add(VertexID v1, VertexID v2);

            /// Add contributions from multiple inter-region connections.
            ///
            /// \param[in] maxRowIdx Maximum row (source region) index
            ///    across all new inter-region connection contributions.
            ///
            /// \param[in] maxColIdx Maximum column (destination region)
            ///    index across all new inter-region contributions.
            ///
            /// \param[in] rows Source region indices for all new
            ///    inter-region connection contributions.
            ///
            /// \param[in] cols Destination region indices for all new
            ///    inter-region connection contributions.
            void add(VertexID          maxRowIdx,
                     VertexID          maxColIdx,
                     const Neighbours& rows,
                     const Neighbours& cols);

            /// Clear internal tables.  Preserve allocated capacity.
            void clear();

            /// Predicate.
            ///
            /// \return Whether or not internal tables are empty.
            bool empty() const;

            /// Whether or not internal tables meet size consistency
            /// requirements.
            bool isValid() const;

            /// Maximum zero-based row index.
            std::optional<BaseVertexID> maxRow() const;

            /// Maximum zero-based column index.
            std::optional<BaseVertexID> maxCol() const;

            /// Number of uncompressed contributions in internal tables.
            typename Neighbours::size_type numContributions() const;

            /// Read-only access to uncompressed row indices.
            const Neighbours& rowIndices() const;

            /// Read-only access to uncompressed column indices.
            const Neighbours& columnIndices() const;

            /// Helper function to get the final vertex ID after all merges
            VertexID findMergedVertexID(VertexID v, const std::unordered_map<VertexID, VertexID>& vertex_merges) const;

            /// Apply vertex merges to the stored connections and create compact numbering
            std::unordered_map<VertexID, VertexID> applyVertexMerges(const std::unordered_map<VertexID, VertexID>& vertex_merges);

        private:
            /// Zero-based row/source region indices.
            Neighbours i_{};

            /// Zero-based column/destination region indices.
            Neighbours j_{};

            /// Maximum row index in \code this->i_ \endcode.
            std::optional<VertexID> max_i_{};

            /// Maximum column index in \code this->j_ \endcode.
            std::optional<VertexID> max_j_{};
        };

        /// Compressed sparse row representation of inter-region flow rates
        ///
        /// Row and column indices are zero-based vertex IDs.  Column
        /// indices ascendingly sorted per row.
        class CSR
        {
        public:
            /// Merge coordinate format into existing CSR map.
            ///
            /// \param[in] conns Coordinate representation of new
            ///    contributions.
            ///
            /// \param[in] maxNumVertices Maximum number of vertices.
            ///
            /// \param[in] expandExistingIdxMap Whether or not preserve and
            ///    update the existing compressed index map.  This is
            ///    potentially useful for the case of adding new connections
            ///    to an already compressed graph.  The default setting, \c
            ///    false, will disregard any existing index map and create
            ///    the index map from scratch.  This runtime parameter is
            ///    unused if the class is not configured to track compressed
            ///    indices through the TrackCompressedIdx class parameter.
            void merge(const Connections& conns,
                       const Offset       maxNumVertices,
                       const bool         expandExistingIdxMap);

            /// Total number of rows in compressed map structure.
            Offset numRows() const;

            /// Maximum zero-based row index encountered in mapped structure.
            BaseVertexID maxRowID() const;

            /// Maximum zero-based column index encountered in mapped structure.
            BaseVertexID maxColID() const;

            /// Read-only access to compressed structure's start pointers.
            const Start& startPointers() const;

            /// Read-only access to compressed structure's column indices,
            /// ascendingly sorted per rwo.
            const Neighbours& columnIndices() const;

            /// Coordinate format row index vector.  Expanded from \code
            /// startPointers() \endcode.
            Neighbours coordinateFormatRowIndices() const;

            template <typename Ret = const Start&>
            std::enable_if_t<TrackCompressedIdx, Ret> compressedIndexMap() const
            {
                return this->compressedIdx_;
            }

            // MessageBufferType API should be similar to Dune::MessageBufferIF
            template <class MessageBufferType>
            void write(MessageBufferType& buffer) const
            {
                this->writeVector(this->ia_, buffer);
                this->writeVector(this->ja_, buffer);

                if constexpr (TrackCompressedIdx) {
                    this->writeVector(this->compressedIdx_, buffer);
                }

                buffer.write(this->numRows_);
                buffer.write(this->numCols_);
            }

            // MessageBufferType API should be similar to Dune::MessageBufferIF
            template <class MessageBufferType>
            void read(MessageBufferType& buffer)
            {
                this->readVector(buffer, this->ia_);
                this->readVector(buffer, this->ja_);

                if constexpr (TrackCompressedIdx) {
                    this->readVector(buffer, this->compressedIdx_);
                }

                buffer.read(this->numRows_);
                buffer.read(this->numCols_);
            }

            /// Clear internal tables.  Preserve allocated capacity.
            void clear();

        private:
            struct EmptyPlaceHolder {};

            /// Start pointers.
            Start ia_{};

            /// Column indices.  Ascendingly sorted per row once structure
            /// is fully established.
            Neighbours ja_{};

            /// Destination index in compressed representation.  Vector of
            /// size equal to number of \c addConnection() calls if client
            /// code requests that compressed indices be tracked (i.e., when
            /// parameter TrackCompressedIdx == true); Empty structure
            /// otherwise (default setting).
            std::conditional_t<TrackCompressedIdx, Start, EmptyPlaceHolder> compressedIdx_{};

            /// Number of active rows in compressed map structure.
            BaseVertexID numRows_{};

            /// Number of active columns in compressed map structure.
            /// Tracked as the maximum column index plus one.
            BaseVertexID numCols_{};

            // ---------------------------------------------------------
            // Implementation of read()/write()
            // ---------------------------------------------------------

            template <typename T, class A, class MessageBufferType>
            void writeVector(const std::vector<T,A>& vec,
                             MessageBufferType&      buffer) const
            {
                const auto n = vec.size();
                buffer.write(n);

                for (const auto& x : vec) {
                    buffer.write(x);
                }
            }

            template <class MessageBufferType, typename T, class A>
            void readVector(MessageBufferType& buffer,
                            std::vector<T,A>&  vec)
            {
                auto n = 0 * vec.size();
                buffer.read(n);

                vec.resize(n);

                for (auto& x : vec) {
                    buffer.read(x);
                }
            }

            // ---------------------------------------------------------
            // Implementation of merge()
            // ---------------------------------------------------------

            /// Incorporate new, coordinate format contributions into
            /// existing, possibly empty, CSR mapping structure.
            ///
            /// On exit the ia_ array holds the proper start pointers while
            /// ja_ holds the corresponding column indices albeit possibly
            /// repeated and unsorted.
            ///
            /// \param[in] rows Row indices of all, possibly repeated,
            ///    coordinate format input contributions.  Start pointers \c
            ///    ia_ updated to account for new entries.
            ///
            /// \param[in] cols Column index of coordinate format intput
            ///    structure.  Inserted into \c ja_ according to its
            ///    corresponding row index.
            ///
            /// \param[in] maxRowID Maximum index in \p rows.  Needed to
            ///    ensure proper size of \c ia_.
            ///
            /// \param[in] maxColID Maximum index in \p cols.
            ///
            /// \param[in] expandExistingIdxMap Whether or not preserve and
            ///    update the existing compressed index map.  This is
            ///    potentially useful for the case of adding new connections
            ///    to an already compressed graph.  The default setting, \c
            ///    false, will disregard any existing index map and create
            ///    the index map from scratch.  This runtime parameter is
            ///    unused if the class is not configured to track compressed
            ///    indices through the TrackCompressedIdx class parameter.
            void assemble(const Neighbours& rows,
                          const Neighbours& cols,
                          BaseVertexID      maxRowID,
                          BaseVertexID      maxColID,
                          bool              expandExistingIdxMap);

            /// Sort column indices per row and compress repeated column
            /// indices down to a single unique element per row.  Sum
            /// repeated values
            ///
            /// On exit the \c ia_ and \c ja_ arrays all have their
            /// expected, canonical structure.
            ///
            /// \param[in] maxNumVertices Maximum number of vertices
            ///    supported by final compressed mapping structure.  Ignored
            ///    if less than active number of rows.
            void compress(const Offset maxNumVertices);

            /// Sort column indices within each mapped row.
            ///
            /// On exit \c ja_ has ascendingly sorted column indices, albeit
            /// possibly with repeated entries.  This function also updates
            /// \c compressedIdx_, if applicable, to account for the new
            /// locations of the non-zero elements in the grouped structure.
            void sortColumnIndicesPerRow();

            /// Condense repeated column indices per row down to a single
            /// unique entry for each.
            ///
            /// Assumes that each row has ascendingly sorted column indices
            /// in \c ja_ and must therefore be called after member function
            /// sortColumnIndicesPerRow().  On exit, \c ja_ has its final
            /// canonical structure and \c compressedIdx_, if applicable,
            /// knows the final location of each non-zero contribution in
            /// the input coordinate format.
            void condenseDuplicates();

            // ---------------------------------------------------------
            // Implementation of assemble()
            // ---------------------------------------------------------

            /// Position end pointers at start of row to prepare for column
            /// index grouping by corresponding row index.
            ///
            /// Also counts total number of non-zero elements, possibly
            /// including repetitions, in \code this->ia_[0] \endcode.
            ///
            /// \param[in] numRows Number of rows in final compressed
            ///    structure.  Used to allocate \code this->ia_ \endcode.
            ///
            /// \param[in] rowIdx Row indices of all, possibly repeated,
            ///    coordinate format input contributions.  Needed to count
            ///    the number of possibly repeated column index entries per
            ///    row.
            void preparePushbackRowGrouping(const int         numRows,
                                            const Neighbours& rowIdx);

            /// Group column indices by corresponding row index and track
            /// grouped location of original coordinate format element
            ///
            /// Appends grouped location to \c compressedIdx_ if needed.
            ///
            /// \param[in] rowIdx Row index of coordinate format input
            ///    structure.  Used as grouping key.
            ///
            /// \param[in] colIdx Column index of coordinate format intput
            ///    structure.  Inserted into \c ja_ according to its
            ///    corresponding row index.
            void groupAndTrackColumnIndicesByRow(const Neighbours& rowIdx,
                                                 const Neighbours& colIdx);

            // ---------------------------------------------------------
            // General utilities
            // ---------------------------------------------------------

            /// Transpose connectivity structure.
            ///
            /// Essentially swaps the roles of rows and columns.  Also used
            /// as a basic building block for sortColumnIndicesPerRow().
            void transpose();

            /// Condense sequences of repeated column indices in a single
            /// map row down to a single copy of each unique column index.
            ///
            /// Appends new unique column indices to \code ja_ \endcode
            ///
            /// Assumes that the map row has ascendingly sorted column
            /// indices and therefore has the same requirements as
            /// std::unique.  Will also update the internal compressedIdx_
            /// mapping, if needed, to record new compressed locations for
            /// the current, uncompressed, non-zero map elements.
            ///
            /// \param[in] begin Start of map row that contains possibly
            ///    repeated column indices.
            ///
            /// \param[in] end One-past-end of map row that contains
            ///    possibly repeated column indices.
            void condenseAndTrackUniqueColumnsForSingleRow(typename Neighbours::const_iterator begin,
                                                           typename Neighbours::const_iterator end);

            /// Update \c compressedIdx_ mapping, if needed, to account for
            /// column index reshuffling.
            ///
            /// \param[in] compressedIdx New compressed index locations of
            ///   the non-zero map entries.
            ///
            /// \param[in] numOrigNNZ Number of existing, unique NNZs
            ///   (edges) in graph.  Needed to support calling add() after
            ///   compress() when TrackCompressedIdx is true.
            void remapCompressedIndex(Start&&                                  compressedIdx,
                                      std::optional<typename Start::size_type> numOrigNNZ = std::nullopt);

        };

        /// Accumulated coordinate format contributions that have not yet
        /// been added to the final CSR structure.
        Connections uncompressed_;

        /// Canonical representation of unique inter-region flow rates.
        CSR csr_;

        /// Disjoint-set union parent pointers
        std::unordered_map<VertexID, VertexID> parent_{};

        /// Mapping from original vertex IDs to final vertex IDs after merging and renumbering
        std::unordered_map<VertexID, VertexID> vertex_mapping_{};

        /// Find the root of a disjoint set with path compression
        VertexID find(VertexID v);

        /// Union two sets by rank (using vertex ID as a simple rank)
        void unionSets(VertexID a, VertexID b);
    };

}} // namespace Opm::utility

// Actual implementation of member functions in _impl.hpp file.
#include <opm/common/utility/CSRGraphFromCoordinates_impl.hpp>

#endif // OPM_UTILITY_CSRGRAPHFROMCOORDINATES_HPP
