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
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

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

    // =======================================================================

    class RFTRecord
    {
    public:
        explicit RFTRecord(const std::size_t nconn = 0);

        void collectRecordData(const ::Opm::UnitSystem&  usys,
                               const ::Opm::EclipseGrid& grid,
                               const ::Opm::Well&        well,
                               const ::Opm::data::Well&  wellSol);

        std::size_t nConn() const { return this->i_.size(); }

        void write(::Opm::EclIO::OutputStream::RFT& rftFile) const;

    private:
        std::vector<int> i_;
        std::vector<int> j_;
        std::vector<int> k_;

        std::vector<float> depth_;
        std::vector<float> press_;
        std::vector<float> swat_;
        std::vector<float> sgas_;

        std::vector<Opm::EclIO::PaddedOutputString<8>> host_;

        void addConnection(const ::Opm::UnitSystem&       usys,
                           const ::Opm::Connection&       conn,
                           const ::Opm::data::Connection& xcon,
                           const double                   depth);
    };

    RFTRecord::RFTRecord(const std::size_t nconn)
    {
        if (nconn == 0) { return; }

        this->i_.reserve(nconn);
        this->j_.reserve(nconn);
        this->k_.reserve(nconn);

        this->depth_.reserve(nconn);
        this->press_.reserve(nconn);
        this->swat_ .reserve(nconn);
        this->sgas_ .reserve(nconn);

        this->host_.reserve(nconn);
    }

    void RFTRecord::collectRecordData(const ::Opm::UnitSystem&  usys,
                                      const ::Opm::EclipseGrid& grid,
                                      const ::Opm::Well&        well,
                                      const ::Opm::data::Well&  wellSol)
    {
        const auto& xcon = wellSol.connections;

        for (const auto& connection : well.getConnections()) {
            const auto ix = connection.global_index();

            if (! grid.cellActive(ix)) {
                // Inactive cell.  Ignore.
                continue;
            }

            auto xconPos = std::find_if(xcon.begin(), xcon.end(),
                [ix](const ::Opm::data::Connection& c)
            {
                return c.index == ix;
            });

            if (xconPos == xcon.end()) {
                // RFT data not available for this connection.  Unexpected.
                continue;
            }

            this->addConnection(usys, connection, *xconPos, grid.getCellDepth(ix));
        }
    }

    void RFTRecord::write(::Opm::EclIO::OutputStream::RFT& rftFile) const
    {
        rftFile.write("CONIPOS", this->i_);
        rftFile.write("CONJPOS", this->j_);
        rftFile.write("CONKPOS", this->k_);

        rftFile.write("HOSTGRID", this->host_);

        rftFile.write("DEPTH"   , this->depth_);
        rftFile.write("PRESSURE", this->press_);
        rftFile.write("SWAT"    , this->swat_);
        rftFile.write("SGAS"    , this->sgas_);
    }

    void RFTRecord::addConnection(const ::Opm::UnitSystem&       usys,
                                  const ::Opm::Connection&       conn,
                                  const ::Opm::data::Connection& xcon,
                                  const double                   depth)
    {
        this->i_.push_back(conn.getI() + 1);
        this->j_.push_back(conn.getJ() + 1);
        this->k_.push_back(conn.getK() + 1);

        using M = ::Opm::UnitSystem::measure;
        auto cvrt = [&usys](const M meas, const double x) -> float
        {
            return usys.from_si(meas, x);
        };

        this->depth_.push_back(cvrt(M::length  , depth));
        this->press_.push_back(cvrt(M::pressure, xcon.cell_pressure));

        this->swat_.push_back(xcon.cell_saturation_water);
        this->sgas_.push_back(xcon.cell_saturation_gas);

        this->host_.emplace_back();
    }

    // =======================================================================

    class WellRFTOutputData
    {
    public:
        enum class DataTypes {
            RFT,
        };

        explicit WellRFTOutputData(const std::vector<DataTypes>&                types,
                                   const double                                 elapsed,
                                   const ::Opm::RestartIO::InteHEAD::TimePoint& timePoint,
                                   const ::Opm::UnitSystem&                     usys,
                                   const ::Opm::EclipseGrid&                    grid,
                                   const ::Opm::Well&                           well);

        void addDynamicData(const Opm::data::Well& wellSol);

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

        std::optional<RFTRecord> rft_{};

        std::vector<DataHandler>  dataHandlers_{};
        std::vector<RecordWriter> recordWriters_{};

        static std::map<DataTypes, CreateTypeHandler> creators_;

        void initialiseRFTHandlers();
        bool haveOutputData() const;
        bool haveRFTData() const;

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
        return this->haveRFTData();
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

    void WellRFTOutputData::initialiseRFTHandlers()
    {
        if (this->well_.get().getConnections().empty()) {
            return;
        }

        this->rft_ = RFTRecord{ this->well_.get().getConnections().size() };

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

    bool WellRFTOutputData::haveRFTData() const
    {
        return this->rft_.has_value()
            && (this->rft_->nConn() > std::size_t{0});
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
    };
} // Anonymous namespace

// ===========================================================================

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
        // RFT not yet activated.  Nothing to do.
        return;
    }

    const auto timePoint = ::Opm::RestartIO::
        getSimulationTimePoint(schedule.getStartTime(), elapsed);

    for (const auto& wname : schedule.wellNames(reportStep)) {
        auto rftTypes = std::vector<WellRFTOutputData::DataTypes>{};

        if (rftCfg.rft(wname) || rftCfg.plt(wname)) {
            rftTypes.push_back(WellRFTOutputData::DataTypes::RFT);
        }

        if (rftTypes.empty()) {
            // RFT output not requested for 'wname' at this time.
            continue;
        }

        auto xwPos = wellSol.find(wname);
        if (xwPos == wellSol.end()) {
            // No dynamic data available for 'wname' at this time.
            continue;
        }

        // RFT output requested for 'wname' at this time and dynamic data is
        // available.  Collect requisite information.
        auto rftOutput = WellRFTOutputData {
            rftTypes, elapsed, timePoint, usys, grid,
            schedule[reportStep].wells(wname)
        };

        rftOutput.addDynamicData(xwPos->second);

        // Emit RFT record for 'wname'.  This transparently handles wells
        // without connections--e.g., if the well is only connected in
        // inactive/deactivated cells.
        rftOutput.write(rftFile);
    }
}
