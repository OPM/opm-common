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

#ifndef OPM_OUTPUT_DATA_INTERREGFLOWMAP_HPP
#define OPM_OUTPUT_DATA_INTERREGFLOWMAP_HPP

#include <opm/output/data/InterRegFlow.hpp>

#include <opm/common/utility/CSRGraphFromCoordinates.hpp>

#include <cstddef>
#include <optional>
#include <utility>
#include <type_traits>
#include <vector>

/// \file
///
/// Facility for converting collection of region ID pairs into a sparse
/// (CSR) adjacency matrix representation of a graph.  Supports O(nnz)
/// compression and, if applicable, accumulation of weight values for
/// repeated entity pairs.

namespace Opm { namespace data {

    /// Form CSR adjacency matrix representation of inter-region flow rate
    /// graph provided as a list of connections between regions.
    class InterRegFlowMap
    {
    private:
        /// Representation of neighbouring regions.
        using Neighbours = std::vector<int>;

        /// Offset into neighbour array.
        using Offset = Neighbours::size_type;

        /// CSR start pointers.
        using Start = std::vector<Offset>;

        /// Linear flow rate buffer.
        using RateBuffer = std::vector<float>;

        /// Internal view of flows between regions.
        using Window = InterRegFlow<RateBuffer::iterator>;

    public:
        /// Client view of flows between specified region pair.
        using ReadOnlyWindow = InterRegFlow<std::vector<float>::const_iterator>;

        /// Client type through which to define a single inter-region connection.
        using FlowRates = Window::FlowRates;

        /// Client type through which to identify a component flow of a
        /// single inter-region connection.
        using Component = Window::Component;

        /// Add flow rate connection between regions.
        ///
        /// \param[in] r1 Primary (source) zero-based region index.  Used as
        ///    row index.
        ///
        /// \param[in] r2 Secondary (sink) zero-based region index.  Used as
        ///   column index.
        ///
        /// \param[in] rates Flow rates associated to single connection.
        ///
        /// If both region IDs are the same then this function does nothing.
        void addConnection(const int r1, const int r2, const FlowRates& rates);

        /// Form CSR adjacency matrix representation of input graph from
        /// connections established in previous calls to addConnection().
        ///
        /// \param[in] numRegions Number of rows in resulting CSR matrix.
        ///     If prior calls to addConnection() supply source entity IDs
        ///     (row indices) greater than or equal to \p numRows, then
        ///     method compress() will throw \code std::invalid_argument
        ///     \endcode.
        void compress(const std::size_t numRegions);

        /// Retrieve number of rows (source entities) in input graph.
        /// Corresponds to value of argument passed to compress().  Valid
        /// only after calling compress().
        Offset numRegions() const;

        /// Retrieve accumulated inter-region flow rates for identified pair
        /// of regions.
        ///
        /// \param[in] r1 Primary (source) zero-based region index.  Used as
        ///    row index.
        ///
        /// \param[in] r2 Secondary (sink) zero-based region index.  Used as
        ///    column index.
        ///
        /// \return View of accumulated inter-region flow rates and
        ///    associated flow direction sign.  \code std::nullopt \endcode
        ///    if no such rates exist.
        std::optional<std::pair<ReadOnlyWindow, ReadOnlyWindow::ElmT>>
        getInterRegFlows(const int r1, const int r2) const;

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            this->connections_.write(buffer);
            this->writeVector(this->rates_, buffer);
        }

        // MessageBufferType API should be similar to Dune::MessageBufferIF
        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            this->connections_.read(buffer);

            auto rates = RateBuffer{};
            this->readVector(buffer, rates);
            this->appendRates(rates);
        }

        /// Clear all internal buffers, but preserve allocated capacity.
        void clear();

    private:
        // VertexID = int, TrackCompressedIdx = true.
        using Graph = utility::CSRGraphFromCoordinates<int, true>;

        Graph connections_{};
        RateBuffer rates_{};

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

        template <typename T, class A, class MessageBufferType>
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

        template <typename Rates>
        void appendRates(const Rates& rates)
        {
            this->rates_.insert(this->rates_.end(), rates.begin(), rates.end());
        }
    };

}} // namespace Opm::data

#endif // OPM_OUTPUT_DATA_INTERREGFLOWMAP_HPP
