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

#include <algorithm>
#include <cassert>
#include <exception>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------
// Class Opm::utility::CSRGraphFromCoordinates::Connections
// ---------------------------------------------------------------------

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::add(const VertexID v1, const VertexID v2)
{
    this->i_.push_back(v1);
    this->j_.push_back(v2);

    this->max_i_ = std::max(this->max_i_.value_or(BaseVertexID{}), this->i_.back());
    this->max_j_ = std::max(this->max_j_.value_or(BaseVertexID{}), this->j_.back());
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::add(VertexID          maxRowIdx,
                 VertexID          maxColIdx,
                 const Neighbours& rows,
                 const Neighbours& cols)
{
    if (cols.size() != rows.size()) {
        throw std::invalid_argument {
            "Coordinate format column index table size does not match "
            "row index table size"
        };
    }

    this->i_.insert(this->i_.end(), rows .begin(), rows .end());
    this->j_.insert(this->j_.end(), cols .begin(), cols .end());

    this->max_i_ = std::max(this->max_i_.value_or(BaseVertexID{}), maxRowIdx);
    this->max_j_ = std::max(this->max_j_.value_or(BaseVertexID{}), maxColIdx);
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::clear()
{
    this->j_.clear();
    this->i_.clear();

    this->max_i_.reset();
    this->max_j_.reset();
}

template <typename VertexID, bool TrackCompressedIdx>
bool
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::empty() const
{
    return this->i_.empty();
}

template <typename VertexID, bool TrackCompressedIdx>
bool
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::isValid() const
{
    return this->i_.size() == this->j_.size();
}

template <typename VertexID, bool TrackCompressedIdx>
std::optional<typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::BaseVertexID>
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::maxRow() const
{
    return this->max_i_;
}

template <typename VertexID, bool TrackCompressedIdx>
std::optional<typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::BaseVertexID>
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::maxCol() const
{
    return this->max_j_;
}

template <typename VertexID, bool TrackCompressedIdx>
typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Neighbours::size_type
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::numContributions() const
{
    return this->i_.size();
}

template <typename VertexID, bool TrackCompressedIdx>
const typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Neighbours&
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::rowIndices() const
{
    return this->i_;
}

template <typename VertexID, bool TrackCompressedIdx>
const typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Neighbours&
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
Connections::columnIndices() const
{
    return this->j_;
}

// =====================================================================

// ---------------------------------------------------------------------
// Class Opm::utility::CSRGraphFromCoordinates::CSR
// ---------------------------------------------------------------------

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::merge(const Connections& conns,
           const Offset       maxNumVertices,
           const bool         expandExistingIdxMap)
{
    const auto maxRow = conns.maxRow();

    if (maxRow.has_value() &&
        (static_cast<Offset>(*maxRow) >= maxNumVertices))
    {
        throw std::invalid_argument {
            "Number of vertices in input graph (" +
            std::to_string(*maxRow) + ") "
            "exceeds maximum graph size implied by explicit size of "
            "adjacency matrix (" + std::to_string(maxNumVertices) + ')'
        };
    }

    this->assemble(conns.rowIndices(), conns.columnIndices(),
                   maxRow.value_or(BaseVertexID{0}),
                   conns.maxCol().value_or(BaseVertexID{0}),
                   expandExistingIdxMap);

    this->compress(maxNumVertices);
}

template <typename VertexID, bool TrackCompressedIdx>
typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Offset
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::numRows() const
{
    return this->startPointers().empty()
        ? 0 : this->startPointers().size() - 1;
}

template <typename VertexID, bool TrackCompressedIdx>
typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::BaseVertexID
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::maxRowID() const
{
    return this->numRows_ - 1;
}

template <typename VertexID, bool TrackCompressedIdx>
typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::BaseVertexID
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::maxColID() const
{
    return this->numCols_ - 1;
}

template <typename VertexID, bool TrackCompressedIdx>
const typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Start&
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::startPointers() const
{
    return this->ia_;
}

template <typename VertexID, bool TrackCompressedIdx>
const typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Neighbours&
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::columnIndices() const
{
    return this->ja_;
}

template <typename VertexID, bool TrackCompressedIdx>
typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Neighbours
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::coordinateFormatRowIndices() const
{
    auto rowIdx = Neighbours{};

    if (this->ia_.empty()) {
        return rowIdx;
    }

    rowIdx.reserve(this->ia_.back());

    auto row = BaseVertexID{};

    const auto m = this->ia_.size() - 1;
    for (auto i = 0*m; i < m; ++i, ++row) {
        const auto n = this->ia_[i + 1] - this->ia_[i + 0];

        rowIdx.insert(rowIdx.end(), n, row);
    }

    return rowIdx;
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::clear()
{
    this->ia_.clear();
    this->ja_.clear();

    if constexpr (TrackCompressedIdx) {
        this->compressedIdx_.clear();
    }

    this->numRows_ = 0;
    this->numCols_ = 0;
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::assemble(const Neighbours&  rows,
              const Neighbours&  cols,
              const BaseVertexID maxRowIdx,
              const BaseVertexID maxColIdx,
              [[maybe_unused]] const bool expandExistingIdxMap)
{
    [[maybe_unused]] auto compressedIdx = this->compressedIdx_;
    [[maybe_unused]] const auto numOrigNNZ = this->ja_.size();

    auto i = this->coordinateFormatRowIndices();
    i.insert(i.end(), rows.begin(), rows.end());

    auto j = this->ja_;
    j.insert(j.end(), cols.begin(), cols.end());

    const auto thisNumRows = std::max(this->numRows_, maxRowIdx + 1);
    const auto thisNumCols = std::max(this->numCols_, maxColIdx + 1);

    this->preparePushbackRowGrouping(thisNumRows, i);

    this->groupAndTrackColumnIndicesByRow(i, j);

    if constexpr (TrackCompressedIdx) {
        if (expandExistingIdxMap) {
            this->remapCompressedIndex(std::move(compressedIdx), numOrigNNZ);
        }
    }

    this->numRows_ = thisNumRows;
    this->numCols_ = thisNumCols;
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::compress(const Offset maxNumVertices)
{
    if (this->numRows() > maxNumVertices) {
        throw std::invalid_argument {
            "Number of vertices in input graph (" +
            std::to_string(this->numRows()) + ") "
            "exceeds maximum graph size implied by explicit size of "
            "adjacency matrix (" + std::to_string(maxNumVertices) + ')'
        };
    }

    this->sortColumnIndicesPerRow();

    // Must be called *after* sortColumnIndicesPerRow().
    this->condenseDuplicates();

    const auto nRows = this->startPointers().size() - 1;
    if (nRows < maxNumVertices) {
        this->ia_.insert(this->ia_.end(),
                         maxNumVertices - nRows,
                         this->startPointers().back());
    }
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::sortColumnIndicesPerRow()
{
    // Transposition is, in this context, effectively a linear time (O(nnz))
    // bucket insertion procedure.  In other words transposing the structure
    // twice creates a structure with column indices in (ascendingly) sorted
    // order.

    this->transpose();
    this->transpose();
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::condenseDuplicates()
{
    // Note: Must be called *after* sortColumnIndicesPerRow().

    const auto colIdx = this->ja_;
    auto end          = colIdx.begin();

    this->ja_.clear();

    [[maybe_unused]] auto compressedIdx = this->compressedIdx_;
    if constexpr (TrackCompressedIdx) {
        this->compressedIdx_.clear();
    }

    const auto numRows = this->ia_.size() - 1;
    for (auto row = 0*numRows; row < numRows; ++row) {
        auto begin = end;

        std::advance(end, this->ia_[row + 1] - this->ia_[row + 0]);

        const auto q = this->ja_.size();

        this->condenseAndTrackUniqueColumnsForSingleRow(begin, end);

        this->ia_[row + 0] = q;
    }

    if constexpr (TrackCompressedIdx) {
        this->remapCompressedIndex(std::move(compressedIdx));
    }

    // Record final table sizes.
    this->ia_.back() = this->ja_.size();
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::preparePushbackRowGrouping(const int         numRows,
                                const Neighbours& rowIdx)
{
    assert (numRows >= 0);

    this->ia_.assign(numRows + 1, 0);

    // Count number of neighbouring vertices for each row.  Accumulate in
    // "next" bin since we're positioning the end pointers.
    for (const auto& row : rowIdx) {
        this->ia_[row + 1] += 1;
    }

    // Position "end" pointers.
    //
    // After this loop, ia_[i + 1] points to the *start* of the range of the
    // column indices/neighbouring vertices of vertex 'i'.  This, in turn,
    // enables using the statement ja_[ia_[i+1]++] = v in groupAndTrack()
    // to insert vertex 'v' as a neighbour, at the end of the range of known
    // neighbours, *and* advance the end pointer of row/vertex 'i'.  We use
    // ia_[0] as an accumulator for the total number of neighbouring
    // vertices in the graph.
    //
    // Note index range: 1..numRows inclusive.
    for (typename Start::size_type i = 1, n = numRows; i <= n; ++i) {
        this->ia_[0] += this->ia_[i];
        this->ia_[i]  = this->ia_[0] - this->ia_[i];
    }

    assert (this->ia_[0] == rowIdx.size());
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::groupAndTrackColumnIndicesByRow(const Neighbours& rowIdx,
                                     const Neighbours& colIdx)
{
    assert (this->ia_[0] == rowIdx.size());

    const auto nnz = rowIdx.size();

    this->ja_.resize(nnz);

    if constexpr (TrackCompressedIdx) {
        this->compressedIdx_.clear();
        this->compressedIdx_.reserve(nnz);
    }

    // Group/insert column indices according to their associate vertex/row
    // index.
    //
    // At the start of the loop the end pointers ia_[i+1], formed in
    // preparePushback(), are positioned at the *start* of the column index
    // range associated to vertex 'i'.  After this loop all vertices
    // neighbouring vertex 'i' will be placed consecutively, in order of
    // appearance, into ja_.  Furthermore, the row pointers ia_ will have
    // their final position.
    //
    // The statement ja_[ia_[i+1]++] = v, split into two statements using
    // the helper object 'k', inserts 'v' as a neighbouring vertex of vertex
    // 'i' *and* advances the end pointer ia_[i+1] of that vertex.  We use
    // and maintain the invariant that ia_[i+1] at all times records the
    // insertion point of the next neighbouring vertex of vertex 'i'.  When
    // the list of neighbouring vertices for vertex 'i' has been exhausted,
    // ia_[i+1] will hold the start position for in ja_ for vertex i+1.
    for (auto nz = 0*nnz; nz < nnz; ++nz) {
        const auto k = this->ia_[rowIdx[nz] + 1] ++;

        this->ja_[k] = colIdx[nz];

        if constexpr (TrackCompressedIdx) {
            this->compressedIdx_.push_back(k);
        }
    }

    this->ia_[0] = 0;
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::transpose()
{
    [[maybe_unused]] auto compressedIdx = this->compressedIdx_;

    {
        const auto rowIdx = this->coordinateFormatRowIndices();
        const auto colIdx = this->ja_;

        this->preparePushbackRowGrouping(this->numCols_, colIdx);

        // Note parameter order.  Transposition switches role of rows and
        // columns.
        this->groupAndTrackColumnIndicesByRow(colIdx, rowIdx);
    }

    if constexpr (TrackCompressedIdx) {
        this->remapCompressedIndex(std::move(compressedIdx));
    }

    std::swap(this->numRows_, this->numCols_);
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
CSR::condenseAndTrackUniqueColumnsForSingleRow(typename Neighbours::const_iterator begin,
                                               typename Neighbours::const_iterator end)
{
    // We assume that we're only called *after* sortColumnIndicesPerRow()
    // whence duplicate elements appear consecutively in [begin, end).
    //
    // Note: This is essentially the same as std::unique(begin, end) save
    // for the return value and the fact that we additionally record the
    // 'compressedIdx_' mapping.  That mapping enables subsequent, decoupled
    // accumulation of the 'sa_' contributions.

    while (begin != end) {
        // Note: Order of ja_ and compressedIdx_ matters here.

        if constexpr (TrackCompressedIdx) {
            this->compressedIdx_.push_back(this->ja_.size());
        }

        this->ja_.push_back(*begin);

        auto next_unique =
            std::find_if(begin, end, [last = this->ja_.back()]
                         (const auto j) { return j != last; });

        if constexpr (TrackCompressedIdx) {
            // Number of duplicate elements in [begin, next_unique).
            const auto ndup = std::distance(begin, next_unique);

            if (ndup > 1) {
                // Insert ndup - 1 copies of .back() to represent the
                // duplicate pairs in [begin, next_unique).  We subtract one
                // to account for .push_back() above representing *begin.
                this->compressedIdx_.insert(this->compressedIdx_.end(),
                                            ndup - 1,
                                            this->compressedIdx_.back());
            }
        }

        begin = next_unique;
    }
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::CSR::
remapCompressedIndex([[maybe_unused]] Start&&                                  compressedIdx,
                     [[maybe_unused]] std::optional<typename Start::size_type> numOrig)
{
    if constexpr (TrackCompressedIdx) {
        for (auto& i : compressedIdx) {
            i = this->compressedIdx_[i];
        }

        if (numOrig.has_value() && (*numOrig < this->compressedIdx_.size())) {
            // Client called add() after compress().  Remap existing portion
            // of compressedIdx (above), and append new entries (here).
            compressedIdx
                .insert(compressedIdx.end(),
                        this->compressedIdx_.begin() + *numOrig,
                        this->compressedIdx_.end());
        }

        this->compressedIdx_.swap(compressedIdx);
    }
}

// =====================================================================

// ---------------------------------------------------------------------
// Class Opm::utility::CSRGraphFromCoordinates
// ---------------------------------------------------------------------

template <typename VertexID, bool TrackCompressedIdx>
void Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::clear()
{
    this->uncompressed_.clear();
    this->csr_.clear();
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
addConnection(const VertexID v1, const VertexID v2)
{
    if ((v1 < 0) || (v2 < 0)) {
        throw std::invalid_argument {
            "Vertex IDs must be non-negative.  Got (v1,v2) = ("
            + std::to_string(v1) + ", " + std::to_string(v2)
            + ')'
        };
    }

    if (v1 == v2) {
        // Ignore self connections.
        return;
    }

    this->uncompressed_.add(v1, v2);
}

template <typename VertexID, bool TrackCompressedIdx>
void
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::
compress(const Offset maxNumVertices, const bool expandExistingIdxMap)
{
    if (! this->uncompressed_.isValid()) {
        throw std::logic_error {
            "Cannot compress invalid connection list"
        };
    }

    this->csr_.merge(this->uncompressed_, maxNumVertices, expandExistingIdxMap);

    this->uncompressed_.clear();
}

template <typename VertexID, bool TrackCompressedIdx>
typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Offset
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::numVertices() const
{
    return this->csr_.numRows();
}

template <typename VertexID, bool TrackCompressedIdx>
typename Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::Offset
Opm::utility::CSRGraphFromCoordinates<VertexID, TrackCompressedIdx>::numEdges() const
{
    const auto& ia = this->startPointers();

    return ia.empty() ? 0 : ia.back();
}
