/*
  Copyright (c) 2019, 2022 Equinor ASA
  Copyright (c) 2016 Statoil ASA
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEF ICT, Applied Mathematics.
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2015 IRIS AS

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

#include <opm/output/eclipse/WriteRFT.hpp>

#include <opm/io/eclipse/OutputStream.hpp>
#include <opm/io/eclipse/PaddedOutputString.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/output/eclipse/InteHEAD.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/Schedule/RFTConfig.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <cassert>
#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {
    enum etcIx : std::vector<Opm::EclIO::PaddedOutputString<8>>::size_type
    {
        Time      =  0,
        Well      =  1,
        LGR       =  2,
        Depth     =  3,
        Pressure  =  4,
        DataType  =  5,
        WellType  =  6,
        LiqRate   =  7,
        GasRate   =  8,
        ResVRate  =  9,
        Velocity  = 10,
        Reserved  = 11, // Untouched
        Viscosity = 12,
        ConcPlyBr = 13,
        PlyBrRate = 14,
        PlyBrAds  = 15,
    };

    namespace RftUnits {
        namespace exceptions {
            void metric(std::vector<Opm::EclIO::PaddedOutputString<8>>& welletc)
            {
                welletc[etcIx::Depth]     = " METRES";
                welletc[etcIx::Velocity]  = " M/SEC";
            }

            void field(std::vector<Opm::EclIO::PaddedOutputString<8>>& welletc)
            {
                welletc[etcIx::Depth]     = "  FEET";
                welletc[etcIx::Velocity]  = " FT/SEC";
                welletc[etcIx::PlyBrRate] = " LB/DAY";
            }

            void lab(std::vector<Opm::EclIO::PaddedOutputString<8>>& welletc)
            {
                welletc[etcIx::Time]      = "   HR";
                welletc[etcIx::Pressure]  = "  ATMA";
                welletc[etcIx::Velocity]  = " CM/SEC";
                welletc[etcIx::ConcPlyBr] = " GM/SCC";
                welletc[etcIx::PlyBrRate] = " GM/HR";
                welletc[etcIx::PlyBrAds]  = "  GM/GM";
            }

            void pvt_m(std::vector<Opm::EclIO::PaddedOutputString<8>>& welletc)
            {
                // PVT_M is METRIC with pressures in atmospheres.
                metric(welletc);

                welletc[etcIx::Pressure] = "  ATMA";
            }

            void input(std::vector<Opm::EclIO::PaddedOutputString<8>>& welletc)
            {
                welletc[etcIx::Time]      = "  INPUT";
                welletc[etcIx::Depth]     = "  INPUT";
                welletc[etcIx::Pressure]  = "  INPUT";
                welletc[etcIx::LiqRate]   = "  INPUT";
                welletc[etcIx::GasRate]   = "  INPUT";
                welletc[etcIx::ResVRate]  = "  INPUT";
                welletc[etcIx::Velocity]  = "  INPUT";
                welletc[etcIx::Viscosity] = "  INPUT";
                welletc[etcIx::ConcPlyBr] = "  INPUT";
                welletc[etcIx::PlyBrRate] = "  INPUT";
                welletc[etcIx::PlyBrAds]  = "  INPUT";
            }
        } // namespace exceptions

        std::string
        centre(std::string s, const std::string::size_type width = 8)
        {
            if (s.size() >  width) { return s.substr(0, width); }
            if (s.size() == width) { return s; }

            const auto npad = width - s.size();
            const auto left = (npad + 1) / 2;  // ceil(npad / 2)

            return std::string(left, ' ') + s;
        }

        std::string combine(std::string left, std::string right)
        {
            return left + '/' + right;
        }

        void fill(const ::Opm::UnitSystem&                        usys,
                  std::vector<Opm::EclIO::PaddedOutputString<8>>& welletc)
        {
            using M = ::Opm::UnitSystem::measure;

            welletc[etcIx::Time]      = centre(usys.name(M::time));
            welletc[etcIx::Depth]     = centre(usys.name(M::length));
            welletc[etcIx::Pressure]  = centre(usys.name(M::pressure));
            welletc[etcIx::LiqRate]   = centre(usys.name(M::liquid_surface_rate));
            welletc[etcIx::GasRate]   = centre(usys.name(M::gas_surface_rate));
            welletc[etcIx::ResVRate]  = centre(usys.name(M::rate));
            welletc[etcIx::Velocity]  = centre(combine(usys.name(M::length),
                                                       usys.name(M::time)));
            welletc[etcIx::Viscosity] = centre(usys.name(M::viscosity));
            welletc[etcIx::ConcPlyBr] =
                centre(combine(usys.name(M::mass),
                               usys.name(M::liquid_surface_volume)));

            welletc[etcIx::PlyBrRate] = centre(usys.name(M::mass_rate));
            welletc[etcIx::PlyBrAds]  = centre(combine(usys.name(M::mass),
                                                       usys.name(M::mass)));
        }
    } // namespace RftUnits

    std::optional<std::vector<Opm::data::Connection>::const_iterator>
    findConnResults(const std::size_t                         cellIndex,
                    const std::vector<Opm::data::Connection>& xcon)
    {
        auto pos = std::find_if(xcon.begin(), xcon.end(),
                                [cellIndex](const Opm::data::Connection& xc)
                                {
                                    return xc.index == cellIndex;
                                });

        if (pos == xcon.end()) {
            return std::nullopt;
        }

        // C++17 class template argument deduction.
        return std::optional{pos};
    }

    template <typename ConnectionIsActive, typename ConnOp>
    void connectionLoop(const Opm::WellConnections& connections,
                        ConnectionIsActive&&        connectionIsActive,
                        ConnOp&&                    connOp)
    {
        for (auto connPos = connections.begin(), end = connections.end();
             connPos != end; ++connPos)
        {
            if (! connectionIsActive(connPos)) {
                // Inactive cell/connection.  Ignore.
                continue;
            }

            connOp(connPos);
        }
    }

    template <typename ConnOp>
    void connectionLoop(const Opm::WellConnections& connections,
                        const Opm::EclipseGrid&     grid,
                        ConnOp&&                    connOp)
    {
        connectionLoop(connections,
                       [&grid](Opm::WellConnections::const_iterator connPos)
                       { return grid.cellActive(connPos->global_index()); },
                       std::forward<ConnOp>(connOp));
    }

    // =======================================================================

    class WellConnectionRecord
    {
    public:
        explicit WellConnectionRecord(const std::size_t nconn = 0);

        void collectRecordData(const ::Opm::EclipseGrid& grid,
                               const ::Opm::Well&        well);

        void write(::Opm::EclIO::OutputStream::RFT& rftFile) const;

    private:
        std::vector<int> i_;
        std::vector<int> j_;
        std::vector<int> k_;

        std::vector<Opm::EclIO::PaddedOutputString<8>> host_;

        void addConnection(const ::Opm::Connection& conn);
    };

    WellConnectionRecord::WellConnectionRecord(const std::size_t nconn)
    {
        if (nconn == std::size_t{0}) { return; }

        this->i_.reserve(nconn);
        this->j_.reserve(nconn);
        this->k_.reserve(nconn);

        this->host_.reserve(nconn);
    }

    void WellConnectionRecord::collectRecordData(const ::Opm::EclipseGrid& grid,
                                                 const ::Opm::Well&        well)
    {
        using ConnPos = ::Opm::WellConnections::const_iterator;

        connectionLoop(well.getConnections(), grid, [this](ConnPos connPos)
        {
            this->addConnection(*connPos);
        });
    }

    void WellConnectionRecord::write(::Opm::EclIO::OutputStream::RFT& rftFile) const
    {
        rftFile.write("CONIPOS", this->i_);
        rftFile.write("CONJPOS", this->j_);
        rftFile.write("CONKPOS", this->k_);

        rftFile.write("HOSTGRID", this->host_);
    }

    void WellConnectionRecord::addConnection(const ::Opm::Connection& conn)
    {
        this->i_.push_back(conn.getI() + 1);
        this->j_.push_back(conn.getJ() + 1);
        this->k_.push_back(conn.getK() + 1);

        this->host_.emplace_back();
    }

    // =======================================================================

    class RFTRecord
    {
    public:
        explicit RFTRecord(const std::size_t nconn = 0);

        void collectRecordData(const ::Opm::UnitSystem&  usys,
                               const ::Opm::EclipseGrid& grid,
                               const ::Opm::Well&        well,
                               const ::Opm::data::Well&  wellSol);

        std::size_t nConn() const { return this->depth_.size(); }

        void write(::Opm::EclIO::OutputStream::RFT& rftFile) const;

    private:
        std::vector<float> depth_;
        std::vector<float> press_;
        std::vector<float> swat_;
        std::vector<float> sgas_;

        void addConnection(const ::Opm::UnitSystem&       usys,
                           const ::Opm::Connection&       conn,
                           const ::Opm::data::Connection& xcon);
    };

    RFTRecord::RFTRecord(const std::size_t nconn)
    {
        if (nconn == std::size_t{0}) { return; }

        this->depth_.reserve(nconn);
        this->press_.reserve(nconn);
        this->swat_ .reserve(nconn);
        this->sgas_ .reserve(nconn);
    }

    void RFTRecord::collectRecordData(const ::Opm::UnitSystem&  usys,
                                      const ::Opm::EclipseGrid& grid,
                                      const ::Opm::Well&        well,
                                      const ::Opm::data::Well&  wellSol)
    {
        using ConnPos = ::Opm::WellConnections::const_iterator;

        const auto& xcon = wellSol.connections;

        connectionLoop(well.getConnections(), grid,
            [this, &usys, &xcon](ConnPos connPos)
        {
            const auto xconPos =
                findConnResults(connPos->global_index(), xcon);

            if (! xconPos.has_value()) {
                return;
            }

            this->addConnection(usys, *connPos, *xconPos.value());
        });
    }

    void RFTRecord::write(::Opm::EclIO::OutputStream::RFT& rftFile) const
    {
        rftFile.write("DEPTH"   , this->depth_);
        rftFile.write("PRESSURE", this->press_);
        rftFile.write("SWAT"    , this->swat_);
        rftFile.write("SGAS"    , this->sgas_);
    }

    void RFTRecord::addConnection(const ::Opm::UnitSystem&       usys,
                                  const ::Opm::Connection&       conn,
                                  const ::Opm::data::Connection& xcon)
    {
        using M = ::Opm::UnitSystem::measure;
        auto cvrt = [&usys](const M meas, const double x) -> float
        {
            return usys.from_si(meas, x);
        };

        this->depth_.push_back(cvrt(M::length  , conn.depth()));
        this->press_.push_back(cvrt(M::pressure, xcon.cell_pressure));

        this->swat_.push_back(xcon.cell_saturation_water);
        this->sgas_.push_back(xcon.cell_saturation_gas);
    }

    // =======================================================================

    class PLTFlowRate
    {
    public:
        explicit PLTFlowRate(const std::size_t nconn = 0);

        void addConnection(const ::Opm::UnitSystem&  usys,
                           const ::Opm::data::Rates& rates);

        const std::vector<float>& oil() const { return this->oil_; }
        const std::vector<float>& gas() const { return this->gas_; }
        const std::vector<float>& water() const { return this->water_; }

    private:
        std::vector<float> oil_{};
        std::vector<float> gas_{};
        std::vector<float> water_{};
    };

    PLTFlowRate::PLTFlowRate(const std::size_t nconn)
    {
        if (nconn == std::size_t{0}) {
            return;
        }

        this->oil_.reserve(nconn);
        this->gas_.reserve(nconn);
        this->water_.reserve(nconn);
    }

    void PLTFlowRate::addConnection(const ::Opm::UnitSystem&  usys,
                                    const ::Opm::data::Rates& rates)
    {
        using M = Opm::UnitSystem::measure;
        using rt = Opm::data::Rates::opt;

        // Note negative sign on call to rates.get() here.  Flow reports
        // positive injection rates and negative production rates but we
        // need the opposite sign convention for this report.

        this->oil_.push_back(usys.from_si(M::liquid_surface_rate, -rates.get(rt::oil, 0.0)));
        this->gas_.push_back(usys.from_si(M::gas_surface_rate, -rates.get(rt::gas, 0.0)));
        this->water_.push_back(usys.from_si(M::liquid_surface_rate, -rates.get(rt::wat, 0.0)));
    }

    // -----------------------------------------------------------------------

    class PLTRecord
    {
    public:
        explicit PLTRecord(const std::size_t nconn = 0);
        virtual ~PLTRecord() = default;

        void collectRecordData(const ::Opm::UnitSystem&  usys,
                               const ::Opm::EclipseGrid& grid,
                               const ::Opm::Well&        well,
                               const ::Opm::data::Well&  wellSol);

        std::size_t nConn() const { return this->depth_.size(); }

        virtual void write(::Opm::EclIO::OutputStream::RFT& rftFile) const;

    protected:
        using ConnPos = ::Opm::WellConnections::const_iterator;

        virtual void prepareConnections(const ::Opm::Well& well);

        virtual void addConnection(const ::Opm::UnitSystem&       usys,
                                   const ::Opm::Well&             well,
                                   ConnPos                        connPos,
                                   const ::Opm::data::Connection& xcon);

        void assignNextNeighbourID(const int id);

    private:
        PLTFlowRate flow_{};

        std::vector<int> neighbour_id_{};

        std::vector<float> depth_{};
        std::vector<float> pressure_{};
        std::vector<float> trans_{};
        std::vector<float> kh_{};

        void assignNextNeighbourID(ConnPos                       connPos,
                                   const ::Opm::WellConnections& wellConns);
    };

    PLTRecord::PLTRecord(const std::size_t nconn)
        : flow_{nconn}
    {
        if (nconn == std::size_t{0}) {
            return;
        }

        this->neighbour_id_.reserve(nconn);
        this->depth_.reserve(nconn);
        this->pressure_.reserve(nconn);
        this->trans_.reserve(nconn);
        this->kh_.reserve(nconn);
    }

    void PLTRecord::collectRecordData(const ::Opm::UnitSystem&  usys,
                                      const ::Opm::EclipseGrid& grid,
                                      const ::Opm::Well&        well,
                                      const ::Opm::data::Well&  wellSol)
    {
        this->prepareConnections(well);

        const auto& xcon = wellSol.connections;
        connectionLoop(well.getConnections(), grid,
            [this, &usys, &well, &xcon](ConnPos connPos)
        {
            const auto xconPos =
                findConnResults(connPos->global_index(), xcon);

            if (! xconPos.has_value()) {
                return;
            }

            this->addConnection(usys, well, connPos, *xconPos.value());
        });
    }

    void PLTRecord::write(::Opm::EclIO::OutputStream::RFT& rftFile) const
    {
        rftFile.write("CONDEPTH", this->depth_);
        rftFile.write("CONPRES" , this->pressure_);

        rftFile.write("CONORAT", this->flow_.oil());
        rftFile.write("CONWRAT", this->flow_.water());
        rftFile.write("CONGRAT", this->flow_.gas());

        rftFile.write("CONFAC", this->trans_);
        rftFile.write("CONKH" , this->kh_);
        rftFile.write("CONNXT", this->neighbour_id_);
    }

    void PLTRecord::prepareConnections([[maybe_unused]] const ::Opm::Well& well)
    {}

    void PLTRecord::addConnection(const ::Opm::UnitSystem&       usys,
                                  const ::Opm::Well&             well,
                                  ConnPos                        connPos,
                                  const ::Opm::data::Connection& xcon)
    {
        using M = ::Opm::UnitSystem::measure;
        auto cvrt = [&usys](const M meas, const double x) -> float
        {
            return usys.from_si(meas, x);
        };

        // Allocate neighbour ID element
        this->neighbour_id_.push_back(0);

        // Infer neighbour connection in direction of well head.
        this->assignNextNeighbourID(connPos, well.getConnections());

        this->depth_.push_back(cvrt(M::length, connPos->depth()));
        this->pressure_.push_back(cvrt(M::pressure, xcon.pressure));
        this->trans_.push_back(cvrt(M::transmissibility, xcon.trans_factor));
        this->kh_.push_back(cvrt(M::effective_Kh, connPos->Kh()));

        this->flow_.addConnection(usys, xcon.rates);
    }

    void PLTRecord::assignNextNeighbourID(ConnPos                       connPos,
                                          const ::Opm::WellConnections& wellConns)
    {
        auto begin = wellConns.begin();

        if (connPos == begin) {
            // This connection is closest to the well head and there is no
            // neighbour.
            this->assignNextNeighbourID(0);
        }
        else {
            const auto connIdx = std::distance(begin, connPos);
            std::advance(begin, connIdx - 1);

            this->assignNextNeighbourID(static_cast<int>(begin->sort_value()) + 1);
        }
    }

    void PLTRecord::assignNextNeighbourID(const int id)
    {
        this->neighbour_id_.back() = id;
    }

    // -----------------------------------------------------------------------

    class CSRIndexRelation
    {
    public:
        using IndexRange = std::pair<std::vector<int>::const_iterator,
                                     std::vector<int>::const_iterator>;

        template <typename Cmp>
        void build(const std::size_t              size,
                   const int                      minId,
                   const std::function<int(int)>& binId,
                   Cmp&&                          cmp);

        IndexRange bin(const int binId) const
        {
            this->verifyValid(binId);

            return { this->start(binId), this->start(binId + 1) };
        }

        bool empty(const int binId) const
        {
            this->verifyValid(binId);

            return this->start(binId) == this->start(binId + 1);
        }

        std::optional<int> last(const int binId) const
        {
            this->verifyValid(binId);

            if (this->empty(binId)) {
                return {};
            }

            auto end = this->start(binId + 1);
            return *--end;
        }

    private:
        int minId_{std::numeric_limits<int>::max()};
        int maxId_{std::numeric_limits<int>::min()};

        std::vector<std::vector<int>::size_type> pos_{};
        std::vector<int> ix_{};

        std::vector<int>::const_iterator start(const int binId) const
        {
            return this->ix_.begin() + this->pos_[binId - this->minId_];
        }

        bool valid(const int binId) const
        {
            return (binId >= this->minId_)
                && (binId <= this->maxId_);
        }

        void verifyValid(const int binId) const
        {
            if (this->valid(binId)) {
                return;
            }

            throw std::invalid_argument {
                fmt::format("Bin ID {} outside valid range {}..{}",
                            binId, this->minId_, this->maxId_)
            };
        }
    };

    template <typename Cmp>
    void CSRIndexRelation::build(const std::size_t              size,
                                 const int                      minId,
                                 const std::function<int(int)>& binId,
                                 Cmp&&                          cmp)
    {
        if (size == std::size_t{0}) {
            return;
        }

        this->ix_.resize(size);
        std::iota(this->ix_.begin(), this->ix_.end(), 0);
        std::sort(this->ix_.begin(), this->ix_.end(), std::forward<Cmp>(cmp));

        // Sort must respect binId(i1) <= binId(i2)
        auto inconsistentId =
            std::adjacent_find(this->ix_.begin(),
                               this->ix_.end(),
                [&binId](const int i1, const int i2)
            {
                return binId(i1) > binId(i2);
            });

        if (inconsistentId != this->ix_.end()) {
            throw std::invalid_argument {
                "Comparison operator does not honour bin consistency requirement"
            };
        }

        const auto binIdBounds =
            std::minmax_element(this->ix_.begin(), this->ix_.end(),
                                [&binId](const int i1, const int i2)
                                { return binId(i1) < binId(i2); });

        if (binIdBounds.first != this->ix_.end()) {
            this->minId_ = std::min(binId(*binIdBounds.first), minId);
            this->maxId_ = binId(*binIdBounds.second);
        }

        if (this->minId_ < minId) {
            // Not particularly likely, but nevertheless possible.
            throw std::invalid_argument {
                "Bin ID function does not honour minimum ID requirement"
            };
        }

        this->pos_.resize(this->maxId_ - this->minId_ + 1 + 1);
        for (const auto& ix : this->ix_) {
            this->pos_[binId(ix) + 1 - this->minId_] += 1;
        }

        std::partial_sum(this->pos_.begin(), this->pos_.end(), this->pos_.begin());
    }

    // -----------------------------------------------------------------------

    class PLTRecordMSW : public PLTRecord
    {
    public:
        explicit PLTRecordMSW(const std::size_t nconn = 0);

        void write(::Opm::EclIO::OutputStream::RFT& rftFile) const override;

    private:
        class OrderSegments
        {
        public:
            explicit OrderSegments(const ::Opm::WellSegments& wellSegs)
                : wellSegs_{ std::cref(wellSegs) }
            {}

            bool operator()(const int i1, const int i2) const;

        private:
            std::reference_wrapper<const ::Opm::WellSegments> wellSegs_;
        };

        class OrderSegConns
        {
        public:
            explicit OrderSegConns(const ::Opm::WellSegments&    wellSegs,
                                   const ::Opm::WellConnections& wellConns)
                : wellSegs_        { std::cref(wellSegs)  }
                , wellConns_       { std::cref(wellConns) }
                , segOrderedBefore_{ wellSegs }
            {}

            bool operator()(const int i1, const int i2) const;

        private:
            std::reference_wrapper<const ::Opm::WellSegments>    wellSegs_;
            std::reference_wrapper<const ::Opm::WellConnections> wellConns_;
            OrderSegments segOrderedBefore_;

            int segIdx(const int connIdx) const;
            int segNum(const int connIdx) const;
            int brnNum(const int segIx) const;
            double connDistance(const int connIdx) const;
        };

        std::vector<int> segment_id_{};
        std::vector<int> branch_id_{};

        std::vector<float> start_length_{};
        std::vector<float> end_length_{};

        CSRIndexRelation segmentConns_{};

        void prepareConnections(const ::Opm::Well& well) override;

        void addConnection(const ::Opm::UnitSystem&       usys,
                           const ::Opm::Well&             well,
                           ConnPos                        connPos,
                           const ::Opm::data::Connection& xcon) override;

        void initialiseSegmentConns(const ::Opm::WellSegments&    wellSegs,
                                    const ::Opm::WellConnections& wellConns);

        int nextNeighbourConnection(ConnPos                       connPos,
                                    const ::Opm::WellSegments&    wellSegs,
                                    const ::Opm::WellConnections& wellConns) const;
    };

    PLTRecordMSW::PLTRecordMSW(const std::size_t nconn)
        : PLTRecord{ nconn }
    {
        if (nconn == std::size_t{0}) {
            return;
        }

        this->segment_id_.reserve(nconn);
        this->branch_id_.reserve(nconn);
        this->start_length_.reserve(nconn);
        this->end_length_.reserve(nconn);
    }

    void PLTRecordMSW::write(::Opm::EclIO::OutputStream::RFT& rftFile) const
    {
        PLTRecord::write(rftFile);

        rftFile.write("CONLENST", this->start_length_);
        rftFile.write("CONLENEN", this->end_length_);
        rftFile.write("CONSEGNO", this->segment_id_);
        rftFile.write("CONBRNO", this->branch_id_);
    }

    void PLTRecordMSW::prepareConnections(const ::Opm::Well& well)
    {
        PLTRecord::prepareConnections(well);

        this->initialiseSegmentConns(well.getSegments(), well.getConnections());
    }

    void PLTRecordMSW::addConnection(const ::Opm::UnitSystem&       usys,
                                     const ::Opm::Well&             well,
                                     ConnPos                        connPos,
                                     const ::Opm::data::Connection& xcon)
    {
        PLTRecord::addConnection(usys, well, connPos, xcon);

        if (! connPos->attachedToSegment()) {
            this->segment_id_.push_back(0);
            this->branch_id_.push_back(0);
            this->start_length_.push_back(0.0f);
            this->end_length_.push_back(0.0f);
            return;
        }

        {
            const auto id = this->nextNeighbourConnection
                (connPos, well.getSegments(), well.getConnections());

            this->assignNextNeighbourID(id);
        }

        this->segment_id_.push_back(connPos->segment());

        {
            const auto branch = well.getSegments()
                .getFromSegmentNumber(this->segment_id_.back())
                .branchNumber();

            this->branch_id_.push_back(branch);
        }

        auto segLength = [&usys](const double len) -> float
        {
            return usys.from_si(Opm::UnitSystem::measure::length, len);
        };

        const auto& [start_md, end_md] = connPos->perf_range().value();
        this->start_length_.push_back(segLength(start_md));
        this->end_length_.push_back(segLength(end_md));
    }

    void PLTRecordMSW::initialiseSegmentConns(const ::Opm::WellSegments&    wellSegs,
                                              const ::Opm::WellConnections& wellConns)
    {
        const auto minSegNum = 1;

        this->segmentConns_.build(wellConns.size(), minSegNum,
            [&wellConns](const int ix) { return wellConns[ix].segment(); },
            OrderSegConns { wellSegs, wellConns });
    }

    int PLTRecordMSW::nextNeighbourConnection(ConnPos                       connPos,
                                              const ::Opm::WellSegments&    wellSegs,
                                              const ::Opm::WellConnections& wellConns) const
    {
        const auto connIx = std::distance(wellConns.begin(), connPos);
        const auto segNum = connPos->segment();
        const auto topSeg = 1;

        auto connRng = this->segmentConns_.bin(segNum);
        if (connRng.first == connRng.second) {
            throw std::runtime_error {
                "Internal error in segment allocation"
            };
        }

        auto getConnectionId = [&wellConns](const std::size_t ix)
        {
            return wellConns[ix].sort_value() + 1;
        };

        if (*connRng.first != connIx) {
            // Not first connection in `segNum`.  Typical case.  Neighbour
            // is next connection closer to the outlet.
            auto i = std::find(connRng.first, connRng.second, connIx);
            assert (i != connRng.second);
            return getConnectionId(*(i - 1));
        }

        if (segNum == topSeg) {
            // We're first connection in top segment.  No other connection
            // neighbour exists in the direction of the well head.
            return 0;
        }

        // We're first connection in `segNum` so search upwards towards top
        // segment, through outletSegment(), for first non-empty segment and
        // pick the *last* connection in that segment.
        auto out = wellSegs.getFromSegmentNumber(segNum).outletSegment();
        while ((out != topSeg) && this->segmentConns_.empty(out)) {
            out = wellSegs.getFromSegmentNumber(out).outletSegment();
        }

        if (this->segmentConns_.empty(out)) {
            // No other connections closer to well head exist.
            return 0;
        }

        return getConnectionId(this->segmentConns_.last(out).value());
    }

    // -----------------------------------------------------------------------

    // i1 < i2 if one of the following relations hold
    //
    // 1) i1's branch number is smaller than i2's branch number
    // 2) i1 and i2 are on the same branch, but i1 is i2's outlet segment
    // 3) Neither are each other's outlet segments, but i1 is closer to the
    //    well head along the tubing.
    bool PLTRecordMSW::OrderSegments::operator()(const int i1, const int i2) const
    {
        const auto& s1 = this->wellSegs_.get()[i1];
        const auto& s2 = this->wellSegs_.get()[i2];

        const auto b1 = s1.branchNumber();
        const auto b2 = s2.branchNumber();

        if (b1 != b2) {
            // i1 not on same branch as i2.  Order by branch number.
            return b1 < b2;
        }

        if (s2.outletSegment() == s1.segmentNumber()) {
            // i1 is i2's outlet
            return true;
        }

        if (s1.outletSegment() == s2.segmentNumber()) {
            // i2 is i1's outlet
            return false;
        }

        // Neither is each other's outlet.  Order by distance along tubing.
        return s1.totalLength() < s2.totalLength();
    }

    // -----------------------------------------------------------------------

    // i1 < i2 if one of the following relations hold
    //
    // 1) i1's branch number is smaller than i2's branch number
    // 2) i1's segment is ordered before i2's segment on the same branch
    // 3) i1 is ordered before i2 on the same segment
    bool PLTRecordMSW::OrderSegConns::operator()(const int i1, const int i2) const
    {
        const auto si1 = this->segIdx(i1);
        const auto si2 = this->segIdx(i2);

        const auto b1 = this->brnNum(si1);
        const auto b2 = this->brnNum(si2);

        if (b1 != b2) {
            // i1 not on same branch as i2.  Order by branch number.
            return b1 < b2;
        }

        if (si1 != si2) {
            // i1 and i2 on same branch, but not on same segment.  Order by
            // whether or not i1's segment is before i2's segment.
            return this->segOrderedBefore_(si1, si2);
        }

        // If we're here i1 and i2 are on the same segment and,
        // transitively, on the same branch.  Order by tubing distance.
        return this->connDistance(i1) < this->connDistance(i2);
    }

    int PLTRecordMSW::OrderSegConns::segNum(const int connIdx) const
    {
        return this->wellConns_.get()[connIdx].segment();
    }

    int PLTRecordMSW::OrderSegConns::segIdx(const int connIdx) const
    {
        return this->wellSegs_.get().segmentNumberToIndex(this->segNum(connIdx));
    }

    int PLTRecordMSW::OrderSegConns::brnNum(const int segIx) const
    {
        return this->wellSegs_.get()[segIx].branchNumber();
    }

    double PLTRecordMSW::OrderSegConns::connDistance(const int connIdx) const
    {
        return this->wellConns_.get()[connIdx].perf_range()->second;
    }

    // =======================================================================

    class WellRFTOutputData
    {
    public:
        enum class DataTypes {
            RFT, PLT,
        };

        explicit WellRFTOutputData(const std::vector<DataTypes>&                types,
                                   const double                                 elapsed,
                                   const ::Opm::RestartIO::InteHEAD::TimePoint& timePoint,
                                   const ::Opm::UnitSystem&                     usys,
                                   const ::Opm::EclipseGrid&                    grid,
                                   const ::Opm::Well&                           well);

        void addDynamicData(const ::Opm::data::Well& wellSol);

        void write(::Opm::EclIO::OutputStream::RFT& rftFile) const;

    private:
        using DataHandler = std::function<
            void(const Opm::data::Well& wellSol)
        >;

        using RecordWriter = std::function<
            void(::Opm::EclIO::OutputStream::RFT& rftFile)
        >;

        using CreateTypeHandler = void (WellRFTOutputData::*)();

        std::reference_wrapper<const Opm::UnitSystem>  usys_;
        std::reference_wrapper<const Opm::EclipseGrid> grid_;
        std::reference_wrapper<const Opm::Well>        well_;
        double                                         elapsed_{};
        Opm::RestartIO::InteHEAD::TimePoint            timeStamp_{};

        // Note: 'rft_' could be an optional<>, but 'plt_' must be a
        // pointer.  We need run-time polymorphic behaviour for 'plt_'.  We
        // therefore use pointers for all of these members, but mostly for
        // uniformity.
        std::unique_ptr<WellConnectionRecord> wconns_{};
        std::unique_ptr<RFTRecord>            rft_{};
        std::unique_ptr<PLTRecord>            plt_{};

        std::vector<DataHandler>  dataHandlers_{};
        std::vector<RecordWriter> recordWriters_{};

        static std::map<DataTypes, CreateTypeHandler> creators_;

        void initialiseConnHandlers();
        void initialiseRFTHandlers();
        void initialisePLTHandlers();

        bool haveOutputData() const;
        bool haveRFTData() const;
        bool havePLTData() const;

        void writeHeader(::Opm::EclIO::OutputStream::RFT& rftFile) const;

        std::vector<Opm::EclIO::PaddedOutputString<8>> wellETC() const;
        std::string dataTypeString() const;
        std::string wellTypeString() const;
    };

    WellRFTOutputData::WellRFTOutputData(const std::vector<DataTypes>&                types,
                                         const double                                 elapsed,
                                         const ::Opm::RestartIO::InteHEAD::TimePoint& timeStamp,
                                         const ::Opm::UnitSystem&                     usys,
                                         const ::Opm::EclipseGrid&                    grid,
                                         const ::Opm::Well&                           well)
        : usys_     { std::cref(usys) }
        , grid_     { std::cref(grid) }
        , well_     { std::cref(well) }
        , elapsed_  { elapsed         }
        , timeStamp_{ timeStamp       }
    {
        this->initialiseConnHandlers();

        for (const auto& type : types) {
            auto handler = creators_.find(type);
            if (handler == creators_.end()) {
                continue;
            }

            (this->*handler->second)();
        }
    }

    bool WellRFTOutputData::haveOutputData() const
    {
        return this->haveRFTData()
            || this->havePLTData();
    }

    void WellRFTOutputData::addDynamicData(const Opm::data::Well& wellSol)
    {
        for (const auto& handler : this->dataHandlers_) {
            handler(wellSol);
        }
    }

    void WellRFTOutputData::write(::Opm::EclIO::OutputStream::RFT& rftFile) const
    {
        if (! this->haveOutputData()) {
            return;
        }

        this->writeHeader(rftFile);

        for (const auto& recordWriter : this->recordWriters_) {
            recordWriter(rftFile);
        }
    }

    void WellRFTOutputData::initialiseConnHandlers()
    {
        if (this->well_.get().getConnections().empty()) {
            return;
        }

        this->wconns_ = std::make_unique<WellConnectionRecord>
            (this->well_.get().getConnections().size());

        this->dataHandlers_.emplace_back(
            [this]([[maybe_unused]] const Opm::data::Well& wellSol)
        {
            this->wconns_->collectRecordData(this->grid_, this->well_);
        });

        this->recordWriters_.emplace_back(
            [this](::Opm::EclIO::OutputStream::RFT& rftFile)
        {
            this->wconns_->write(rftFile);
        });
    }

    void WellRFTOutputData::initialiseRFTHandlers()
    {
        if (this->well_.get().getConnections().empty()) {
            return;
        }

        this->rft_ = std::make_unique<RFTRecord>
            (this->well_.get().getConnections().size());

        this->dataHandlers_.emplace_back(
            [this](const Opm::data::Well& wellSol)
        {
            this->rft_->collectRecordData(this->usys_, this->grid_,
                                          this->well_, wellSol);
        });

        this->recordWriters_.emplace_back(
            [this](::Opm::EclIO::OutputStream::RFT& rftFile)
        {
            this->rft_->write(rftFile);
        });
    }

    void WellRFTOutputData::initialisePLTHandlers()
    {
        const auto& well = this->well_.get();

        if (well.getConnections().empty()) {
            return;
        }

        this->plt_ = well.isMultiSegment()
            ? std::make_unique<PLTRecordMSW>(well.getConnections().size())
            : std::make_unique<PLTRecord>   (well.getConnections().size());

        this->dataHandlers_.emplace_back(
            [this](const Opm::data::Well& wellSol)
        {
            this->plt_->collectRecordData(this->usys_, this->grid_,
                                          this->well_, wellSol);
        });

        this->recordWriters_.emplace_back(
            [this](::Opm::EclIO::OutputStream::RFT& rftFile)
        {
            this->plt_->write(rftFile);
        });
    }

    bool WellRFTOutputData::haveRFTData() const
    {
        return (this->rft_ != nullptr)
            && (this->rft_->nConn() > std::size_t{0});
    }

    bool WellRFTOutputData::havePLTData() const
    {
        return (this->plt_ != nullptr)
            && (this->plt_->nConn() > std::size_t{0});
    }

    void WellRFTOutputData::writeHeader(::Opm::EclIO::OutputStream::RFT& rftFile) const
    {
        {
            const auto time = this->usys_.get()
                .from_si(::Opm::UnitSystem::measure::time, this->elapsed_);

            rftFile.write("TIME", std::vector<float> {
                static_cast<float>(time)
            });
        }

        rftFile.write("DATE", std::vector<int> {
                this->timeStamp_.day,   // 1..31
                this->timeStamp_.month, // 1..12
                this->timeStamp_.year,
            });

        rftFile.write("WELLETC", this->wellETC());
    }

    std::vector<Opm::EclIO::PaddedOutputString<8>>
    WellRFTOutputData::wellETC() const
    {
        using UT = ::Opm::UnitSystem::UnitType;
        auto ret = std::vector<Opm::EclIO::PaddedOutputString<8>>(16);

        // Note: ret[etcIx::LGR] is well's LGR.  Default constructed
        // (i.e., blank) string is sufficient to represent no LGR.

        ret[etcIx::Well] = this->well_.get().name();

        // 'P' -> PLT, 'R' -> RFT, 'S' -> Segment
        ret[etcIx::DataType] = this->dataTypeString();

        // STANDARD or MULTISEG only.
        ret[etcIx::WellType] = this->wellTypeString();

        RftUnits::fill(this->usys_, ret);

        switch (this->usys_.get().getType()) {
        case UT::UNIT_TYPE_METRIC:
            RftUnits::exceptions::metric(ret);
            break;

        case UT::UNIT_TYPE_FIELD:
            RftUnits::exceptions::field(ret);
            break;

        case UT::UNIT_TYPE_LAB:
            RftUnits::exceptions::lab(ret);
            break;

        case UT::UNIT_TYPE_PVT_M:
            RftUnits::exceptions::pvt_m(ret);
            break;

        case UT::UNIT_TYPE_INPUT:
            RftUnits::exceptions::input(ret);
            break;
        }

        return ret;
    }

    std::string WellRFTOutputData::dataTypeString() const
    {
        auto tstring = std::string{};
        if (this->haveRFTData()) { tstring += 'R';}
        if (this->havePLTData()) { tstring += 'P';}

        return tstring;
    }

    std::string WellRFTOutputData::wellTypeString() const
    {
        return this->well_.get().isMultiSegment()
            ? "MULTISEG"
            : "STANDARD";
    }

    std::map<WellRFTOutputData::DataTypes, WellRFTOutputData::CreateTypeHandler>
    WellRFTOutputData::creators_{
        { WellRFTOutputData::DataTypes::RFT, &WellRFTOutputData::initialiseRFTHandlers },
        { WellRFTOutputData::DataTypes::PLT, &WellRFTOutputData::initialisePLTHandlers },
    };
} // Anonymous namespace

// ===========================================================================

namespace {
    std::vector<WellRFTOutputData::DataTypes>
    rftDataTypes(const Opm::RFTConfig& rft_config,
                 const std::string&    well_name)
    {
        auto rftTypes = std::vector<WellRFTOutputData::DataTypes>{};

        if (rft_config.rft(well_name)) {
            rftTypes.push_back(WellRFTOutputData::DataTypes::RFT);
        }

        if (rft_config.plt(well_name)) {
            rftTypes.push_back(WellRFTOutputData::DataTypes::PLT);
        }

        return rftTypes;
    }
}

void Opm::RftIO::write(const int                        reportStep,
                       const double                     elapsed,
                       const ::Opm::UnitSystem&         usys,
                       const ::Opm::EclipseGrid&        grid,
                       const ::Opm::Schedule&           schedule,
                       const ::Opm::data::Wells&        wellSol,
                       ::Opm::EclIO::OutputStream::RFT& rftFile)
{
    const auto& rftCfg = schedule[reportStep].rft_config();
    if (! rftCfg.active()) {
        // RFT file output not yet activated.  Nothing to do.
        return;
    }

    const auto timePoint = ::Opm::RestartIO::
        getSimulationTimePoint(schedule.getStartTime(), elapsed);

    for (const auto& wname : schedule.wellNames(reportStep)) {
        const auto rftTypes = rftDataTypes(rftCfg, wname);

        if (rftTypes.empty()) {
            // RFT file output not requested for 'wname' at this time.
            continue;
        }

        auto xwPos = wellSol.find(wname);
        if (xwPos == wellSol.end()) {
            // No dynamic data available for 'wname' at this time.
            continue;
        }

        // RFT file output requested for 'wname' at this time and dynamic
        // data is available.  Collect requisite information.
        auto rftOutput = WellRFTOutputData {
            rftTypes, elapsed, timePoint, usys, grid,
            schedule[reportStep].wells(wname)
        };

        rftOutput.addDynamicData(xwPos->second);

        // Emit RFT file output record for 'wname'.  This transparently
        // handles wells without connections--e.g., if the well is only
        // connected in inactive/deactivated cells.
        rftOutput.write(rftFile);
    }
}
