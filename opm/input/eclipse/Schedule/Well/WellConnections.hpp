/*
  Copyright 2013 Statoil ASA.

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

#ifndef CONNECTIONSET_HPP_
#define CONNECTIONSET_HPP_

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Opm {
    class ActiveGridCells;
    class DeckRecord;
    class EclipseGrid;
    class ErrorGuard;
    class FieldPropsManager;
    class KeywordLocation;
    class ParseContext;
    class ScheduleGrid;
    class WDFAC;
    struct WellTrajInfo;
} // namespace Opm

namespace Opm {

    class WellConnections
    {
    public:
        using const_iterator = std::vector<Connection>::const_iterator;

        WellConnections() = default;
        WellConnections(const Connection::Order ordering, const int headI, const int headJ);
        WellConnections(const Connection::Order ordering, const int headI, const int headJ,
                        const std::vector<Connection>& connections);

        static WellConnections serializationTestObject();

        // cppcheck-suppress noExplicitConstructor
        template <class Grid>
        WellConnections(const WellConnections& src, const Grid& grid)
            : m_ordering(src.ordering())
            , headI     (src.headI)
            , headJ     (src.headJ)
        {
            for (const auto& c : src) {
                if (grid.isCellActive(c.getI(), c.getJ(), c.getK())) {
                    this->add(c);
                }
            }
        }

        void add(const Connection& conn)
        {
            this->m_connections.push_back(conn);
        }

        void addConnection(const int i, const int j, const int k,
                           const std::size_t global_index,
                           const Connection::State state,
                           const double depth,
                           const Connection::CTFProperties& ctf_props,
                           const int satTableId,
                           const Connection::Direction direction = Connection::Direction::Z,
                           const Connection::CTFKind ctf_kind = Connection::CTFKind::DeckValue,
                           const std::size_t seqIndex = 0,
                           int lgr_grid_number = 0,
                           const bool defaultSatTabId = true);

        // Out-parameter 'requested_open_complnums' is cleared and then set to
        // the complnum of every existing connection this record opens (assigns
        // OPEN), for raising REQUEST_OPEN_COMPLETION events.  Likewise,
        // 'requested_shut_complnums' is cleared and then set to the complnum of
        // every existing connection this record shuts (assigns a non-OPEN
        // state), for clearing any pending REQUEST_OPEN_COMPLETION events.
        void loadCOMPDAT(const DeckRecord&      record,
                         const std::string&     wname,
                         const WDFAC&           wdfac,
                         const ScheduleGrid&    grid,
                         const KeywordLocation& location,
                         const ParseContext&    parseContext,
                         ErrorGuard&            errors,
                         std::vector<int>&      requested_open_complnums,
                         std::vector<int>&      requested_shut_complnums);

        void loadCOMPDATL(const DeckRecord&      record,
                          const std::string&     wname,
                          const WDFAC&           wdfac,
                          const ScheduleGrid&    grid,
                          const KeywordLocation& location,
                          const ParseContext&    parseContext,
                          ErrorGuard&            errors,
                          std::vector<int>&      requested_open_complnums,
                          std::vector<int>&      requested_shut_complnums);

        void loadCOMPTRAJ(const DeckRecord&      record,
                          const std::string&     wname,
                          const ScheduleGrid&    grid,
                          const KeywordLocation& location,
                          WellTrajInfo&          wellTraj);

        void loadWELTRAJ(const DeckRecord&      record,
                         const std::string&     wname,
                         const ScheduleGrid&    grid,
                         const KeywordLocation& location);

        void applyDFactorCorrelation(const ScheduleGrid& grid,
                                     const WDFAC&        wdfac);

        int getHeadI() const;
        int getHeadJ() const;
        const std::vector<double>& getMD() const;
        std::size_t size() const;
        bool empty() const;
        std::size_t num_open() const;
        const Connection& operator[](std::size_t index) const;
        const Connection& get(std::size_t index) const;
        const Connection& getFromIJK(const int i, const int j, const int k) const;
        const Connection& getFromGlobalIndex(std::size_t global_index) const;
        const Connection& lowest() const;
        Connection& getFromIJK(const int i, const int j, const int k);
        Connection* maybeGetFromGlobalIndex(const std::size_t global_index);
        bool hasGlobalIndex(std::size_t global_index) const;
        double segment_perf_length(int segment) const;

        const_iterator begin() const { return this->m_connections.begin(); }
        const_iterator end() const { return this->m_connections.end(); }
        auto begin() { return this->m_connections.begin(); }
        auto end() { return this->m_connections.end(); }
        bool allConnectionsShut() const;

        /// Order connections along well bore.
        ///
        /// There are three distinct cases.
        ///
        ///  -# Multi-segmented wells order connections according to branch
        ///     ID and measured depths along the branch.
        ///
        ///  -# TRACK ordering (COMPORD keyword, default) orders connections
        ///     starting from the well heel using a path travelling salesman
        ///     algorithm with 2-opt ordering and directional penalties.
        ///
        ///  -# DEPTH ordering (COMPORD keyword) orders the connections
        ///     according to the block centre point depths.
        ///
        /// This is a somewhat expensive operation that should typically be
        /// invoked only at the end of applying all pertinent connection updates.
        void order();

        bool operator==( const WellConnections& ) const;
        bool operator!=( const WellConnections& ) const;

        Connection::Order ordering() const { return this->m_ordering; }
        std::vector<const Connection *> output(const EclipseGrid& grid) const;

        /// Activate or reactivate WELPI scaling for this connection set.
        ///
        /// Following this call, any WELPI-based scaling will apply to all
        /// connections whose properties are not reset in COMPDAT.
        ///
        /// Returns whether or not this call to prepareWellPIScaling() is
        /// a state change (e.g., no WELPI to active WELPI or WELPI for
        /// some connections to WELPI for all connections).
        bool prepareWellPIScaling();

        /// Scale pertinent connections' CF value by supplied value.  Scaling
        /// factor typically derived from 'WELPI' input keyword and a dynamic
        /// productivity index calculation.  Applicability array specifies
        /// whether or not a particular connection is exempt from scaling.
        /// Empty array means "apply scaling to all eligible connections".
        /// This array is updated on return (entries set to 'false' if
        /// corresponding connection is not eligible).
        void applyWellPIScaling(const double       scaleFactor,
                                std::vector<bool>& scalingApplicable);

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->m_ordering);
            serializer(this->headI);
            serializer(this->headJ);
            serializer(this->m_connections);
            serializer(this->coord);
            serializer(this->md);
        }

    private:
        Connection::Order m_ordering { Connection::Order::TRACK };
        int headI{0};
        int headJ{0};
        std::vector<Connection> m_connections{};

        std::array<std::vector<double>, 3> coord{};
        std::vector<double> md{};

        void addConnection(const int i, const int j, const int k,
                           const std::size_t global_index,
                           const int complnum,
                           const Connection::State state,
                           const double depth,
                           const Connection::CTFProperties& ctf_props,
                           const int satTableId,
                           const Connection::Direction direction,
                           const Connection::CTFKind ctf_kind,
                           const std::size_t seqIndex,
                           int lgr_grid_number,
                           const bool defaultSatTabId);

        void orderTRACK();
        void orderMSW();
        void orderDEPTH();

        void loadCOMPDATX(const DeckRecord&                 record,
                          const std::string&                wname,
                          const WDFAC&                      wdfac,
                          const ScheduleGrid&               grid,
                          const KeywordLocation&            location,
                          const std::optional<std::string>& lgr_label,
                          const ParseContext&               parseContext,
                          ErrorGuard&                       errors,
                          std::vector<int>&                 requested_open_complnums,
                          std::vector<int>&                 requested_shut_complnums);
    };

    std::optional<int>
    getCompletionNumberFromGlobalConnectionIndex(const WellConnections& connections,
                                                 const std::size_t      global_index);
} // namespace Opm

#endif // CONNECTIONSET_HPP_
