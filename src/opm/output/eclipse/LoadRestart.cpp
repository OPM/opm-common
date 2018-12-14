/*
  Copyright (c) 2018 Equinor ASA
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

#include <opm/output/eclipse/RestartIO.hpp>

#include <opm/output/eclipse/RestartValue.hpp>

#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/doubhead.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/msw.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/output/eclipse/libECLRestart.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

class RestartFileView
{
public:
    explicit RestartFileView(const std::string& filename,
                             const int          report_step);

    ~RestartFileView() = default;

    RestartFileView(const RestartFileView& rhs) = delete;
    RestartFileView(RestartFileView&& rhs);

    RestartFileView& operator=(const RestartFileView& rhs) = delete;
    RestartFileView& operator=(RestartFileView&& rhs);

    std::size_t simStep() const
    {
        return this->sim_step_;
    }

    operator const Opm::RestartIO::ecl_file_view_type*() const
    {
        return this->step_view_;
    }

    const Opm::RestartIO::ecl_kw_type* getKeyword(const char* kw) const
    {
        namespace Load = Opm::RestartIO;

        // Main grid only.  Does not handle/support LGR.
        return Load::ecl_file_view_has_kw       (*this, kw)
            ?  Load::ecl_file_view_iget_named_kw(*this, kw, 0)
            :  nullptr;
    }

private:
    using RstFile = Opm::RestartIO::ert_unique_ptr<
        Opm::RestartIO::ecl_file_type,
        Opm::RestartIO::ecl_file_close>;

    std::size_t                         sim_step_;
    RstFile                             rst_file_;
    Opm::RestartIO::ecl_file_view_type* step_view_ = nullptr;

    operator Opm::RestartIO::ecl_file_type*()
    {
        return this->rst_file_.get();
    }

    operator const Opm::RestartIO::ecl_file_type*() const
    {
        return this->rst_file_.get();
    }
};

RestartFileView::RestartFileView(const std::string& filename,
                                 const int          report_step)
    : sim_step_(std::max(report_step - 1, 0))
    , rst_file_(Opm::RestartIO::ecl_file_open(filename.c_str(), 0))
{
    namespace Load = Opm::RestartIO;

    if (this->rst_file_ == nullptr) {
        throw std::invalid_argument {
            "Unable to open Restart File '" + filename
            + "' at Report Step " + std::to_string(report_step)
        };
    }

    this->step_view_ =
        (Load::EclFiletype(filename) == Load::ECL_UNIFIED_RESTART_FILE)
        ? Load::ecl_file_get_restart_view(*this, -1, report_step, -1, -1)
        : Load::ecl_file_get_global_view (*this);    // Separate

    if (this->step_view_ == nullptr) {
        throw std::runtime_error {
            "Unable to acquire restart information for report step "
            + std::to_string(report_step)
        };
    }
}

RestartFileView::RestartFileView(RestartFileView&& rhs)
    : sim_step_ (rhs.sim_step_)            // Scalar (size_t)
    , rst_file_ (std::move(rhs.rst_file_))
    , step_view_(rhs.step_view_)           // Pointer
{}

RestartFileView& RestartFileView::operator=(RestartFileView&& rhs)
{
    this->sim_step_  = rhs.sim_step_;            // Scalar (size_t)
    this->rst_file_  = std::move(rhs.rst_file_);
    this->step_view_ = rhs.step_view_;           // Pointer copy

    return *this;
}

// ---------------------------------------------------------------------

namespace {
    template <typename T>
    const T* getPtr(const ::Opm::RestartIO::ecl_kw_type* kw)
    {
        return (kw == nullptr) ? nullptr
            : static_cast<const T*>(ecl_kw_iget_ptr(kw /* <- ADL */, 0));
    }

    template <typename T>
    boost::iterator_range<const T*>
    getDataWindow(const T*          arr,
                  const std::size_t windowSize,
                  const std::size_t entity,
                  const std::size_t subEntity               = 0,
                  const std::size_t maxSubEntitiesPerEntity = 1)
    {
        const auto off =
            windowSize * (subEntity + maxSubEntitiesPerEntity*entity);

        const auto* begin = arr   + off;
        const auto* end   = begin + windowSize;

        return { begin, end };
    }

    int getInteHeadElem(const ::Opm::RestartIO::ecl_kw_type* intehead,
                        const std::vector<int>::size_type    i)
    {
        return getPtr<int>(intehead)[i];
    }
}

// ---------------------------------------------------------------------

class WellVectors
{
public:
    template <typename T>
    using Window = boost::iterator_range<const T*>;

    explicit WellVectors(const RestartFileView&               rst_view,
                         const ::Opm::RestartIO::ecl_kw_type* intehead);

    bool hasDefinedWellValues() const;
    bool hasDefinedConnectionValues() const;

    Window<int>    iwel(const std::size_t wellID) const;
    Window<double> xwel(const std::size_t wellID) const;

    Window<int>
    icon(const std::size_t wellID, const std::size_t connID) const;

    Window<double>
    xcon(const std::size_t wellID, const std::size_t connID) const;

private:
    std::size_t maxConnPerWell_;
    std::size_t numIWelElem_;
    std::size_t numXWelElem_;
    std::size_t numIConElem_;
    std::size_t numXConElem_;

    const ::Opm::RestartIO::ecl_kw_type* iwel_;
    const ::Opm::RestartIO::ecl_kw_type* xwel_;

    const ::Opm::RestartIO::ecl_kw_type* icon_;
    const ::Opm::RestartIO::ecl_kw_type* xcon_;
};

WellVectors::WellVectors(const RestartFileView&               rst_view,
                         const ::Opm::RestartIO::ecl_kw_type* intehead)
    : maxConnPerWell_(getInteHeadElem(intehead, VI::intehead::NCWMAX))
    , numIWelElem_   (getInteHeadElem(intehead, VI::intehead::NIWELZ))
    , numXWelElem_   (getInteHeadElem(intehead, VI::intehead::NXWELZ))
    , numIConElem_   (getInteHeadElem(intehead, VI::intehead::NICONZ))
    , numXConElem_   (getInteHeadElem(intehead, VI::intehead::NXCONZ))
    , iwel_          (rst_view.getKeyword("IWEL"))
    , xwel_          (rst_view.getKeyword("XWEL"))
    , icon_          (rst_view.getKeyword("ICON"))
    , xcon_          (rst_view.getKeyword("XCON"))
{}

bool WellVectors::hasDefinedWellValues() const
{
    return ! ((this->iwel_ == nullptr) ||
              (this->xwel_ == nullptr));
}

bool WellVectors::hasDefinedConnectionValues() const
{
    return ! ((this->icon_ == nullptr) ||
              (this->xcon_ == nullptr));
}

WellVectors::Window<int>
WellVectors::iwel(const std::size_t wellID) const
{
    if (! this->hasDefinedWellValues()) {
        throw std::logic_error {
            "Cannot Request IWEL Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<int>(this->iwel_),
                         this->numIWelElem_, wellID);
}

WellVectors::Window<double>
WellVectors::xwel(const std::size_t wellID) const
{
    if (! this->hasDefinedWellValues()) {
        throw std::logic_error {
            "Cannot Request XWEL Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<double>(this->xwel_),
                         this->numXWelElem_, wellID);
}

WellVectors::Window<int>
WellVectors::icon(const std::size_t wellID, const std::size_t connID) const
{
    if (! this->hasDefinedConnectionValues()) {
        throw std::logic_error {
            "Cannot Request ICON Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<int>(this->icon_), this->numIConElem_,
                         wellID, connID, this->maxConnPerWell_);
}

WellVectors::Window<double>
WellVectors::xcon(const std::size_t wellID, const std::size_t connID) const
{
    if (! this->hasDefinedConnectionValues()) {
        throw std::logic_error {
            "Cannot Request XCON Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<double>(this->xcon_), this->numXConElem_,
                         wellID, connID, this->maxConnPerWell_);
}

// ---------------------------------------------------------------------

class GroupVectors
{
public:
    template <typename T>
    using Window = boost::iterator_range<const T*>;

    explicit GroupVectors(const RestartFileView&               rst_view,
                          const ::Opm::RestartIO::ecl_kw_type* intehead);

    bool hasDefinedValues() const;

    std::size_t maxGroups() const;

    Window<int>    igrp(const std::size_t groupID) const;
    Window<double> xgrp(const std::size_t groupID) const;

private:
    std::size_t maxNumGroups_;
    std::size_t numIGrpElem_;
    std::size_t numXGrpElem_;

    const ::Opm::RestartIO::ecl_kw_type* igrp_;
    const ::Opm::RestartIO::ecl_kw_type* xgrp_;
};

GroupVectors::GroupVectors(const RestartFileView&               rst_view,
                           const ::Opm::RestartIO::ecl_kw_type* intehead)
    : maxNumGroups_(getInteHeadElem(intehead, VI::intehead::NGMAXZ) - 1) // -FIELD
    , numIGrpElem_ (getInteHeadElem(intehead, VI::intehead::NIGRPZ))
    , numXGrpElem_ (getInteHeadElem(intehead, VI::intehead::NXGRPZ))
    , igrp_        (rst_view.getKeyword("IGRP"))
    , xgrp_        (rst_view.getKeyword("XGRP"))
{}

bool GroupVectors::hasDefinedValues() const
{
    return ! ((this->igrp_ == nullptr) ||
              (this->xgrp_ == nullptr));
}

std::size_t GroupVectors::maxGroups() const
{
    return this->maxNumGroups_;
}

GroupVectors::Window<int>
GroupVectors::igrp(const std::size_t groupID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request IWEL Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<int>(this->igrp_),
                         this->numIGrpElem_, groupID);
}

GroupVectors::Window<double>
GroupVectors::xgrp(const std::size_t groupID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request XGRP Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<double>(this->xgrp_),
                         this->numXGrpElem_, groupID);
}

// ---------------------------------------------------------------------

class SegmentVectors
{
public:
    template <typename T>
    using Window = boost::iterator_range<const T*>;

    explicit SegmentVectors(const RestartFileView&               rst_view,
                            const ::Opm::RestartIO::ecl_kw_type* intehead);

    bool hasDefinedValues() const;

    Window<int>
    iseg(const std::size_t mswID, const std::size_t segID) const;

    Window<double>
    rseg(const std::size_t mswID, const std::size_t segID) const;

private:
    std::size_t maxSegPerWell_;
    std::size_t numISegElm_;
    std::size_t numRSegElm_;

    const ::Opm::RestartIO::ecl_kw_type* iseg_;
    const ::Opm::RestartIO::ecl_kw_type* rseg_;
};

SegmentVectors::SegmentVectors(const RestartFileView&               rst_view,
                               const ::Opm::RestartIO::ecl_kw_type* intehead)
    : maxSegPerWell_(getInteHeadElem(intehead, VI::intehead::NSEGMX))
    , numISegElm_   (getInteHeadElem(intehead, VI::intehead::NISEGZ))
    , numRSegElm_   (getInteHeadElem(intehead, VI::intehead::NRSEGZ))
    , iseg_         (rst_view.getKeyword("ISEG"))
    , rseg_         (rst_view.getKeyword("RSEG"))
{}

bool SegmentVectors::hasDefinedValues() const
{
    return ! ((this->iseg_ == nullptr) ||
              (this->rseg_ == nullptr));
}

SegmentVectors::Window<int>
SegmentVectors::iseg(const std::size_t mswID, const std::size_t segID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request ISEG Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<int>(this->iseg_), this->numISegElm_,
                         mswID, segID, this->maxSegPerWell_);
}

SegmentVectors::Window<double>
SegmentVectors::rseg(const std::size_t mswID, const std::size_t segID) const
{
    if (! this->hasDefinedValues()) {
        throw std::logic_error {
            "Cannot Request RSEG Values Unless Defined"
        };
    }

    return getDataWindow(getPtr<double>(this->rseg_), this->numRSegElm_,
                         mswID, segID, this->maxSegPerWell_);
}

// ---------------------------------------------------------------------

namespace {
    void throwIfMissingRequired(const Opm::RestartKey& rst_key)
    {
        if (rst_key.required) {
            throw std::runtime_error {
                "Requisite restart vector '"
                + rst_key.key +
                "' is not available in restart file"
            };
        }
    }

    std::vector<double>
    double_vector(const ::Opm::RestartIO::ecl_kw_type* ecl_kw)
    {
        namespace Load = ::Opm::RestartIO;

        const auto size = static_cast<std::size_t>(
            Load::ecl_kw_get_size(ecl_kw));

        if (Load::ecl_type_get_type(Load::ecl_kw_get_data_type(ecl_kw)) == Load::ECL_DOUBLE_TYPE) {
            const double* ecl_data = Load::ecl_kw_get_type_ptr<double>(ecl_kw, Load::ECL_DOUBLE_TYPE);

            return { ecl_data , ecl_data + size };
        }
        else {
            const float* ecl_data = Load::ecl_kw_get_type_ptr<float>(ecl_kw, Load::ECL_FLOAT_TYPE);

            return { ecl_data , ecl_data + size };
        }
    }

    std::vector<double>
    getOpmExtraFromDoubHEAD(const RestartFileView& rst_view,
                            const bool             required,
                            const Opm::UnitSystem& usys)
    {
        using M = Opm::UnitSystem::measure;

        const auto* doubhead =
            getPtr<double>(rst_view.getKeyword("DOUBHEAD"));

        const auto TsInit = doubhead[VI::doubhead::TsInit];

        if (TsInit < 0.0) {
            throwIfMissingRequired({ "OPMEXTRA", M::identity, required });
        }

        return { usys.to_si(M::time, TsInit) };
    }

    Opm::data::Solution
    restoreSOLUTION(const RestartFileView&              rst_view,
                    const std::vector<Opm::RestartKey>& solution_keys,
                    const int                           numcells)
    {
        Opm::data::Solution sol(/* init_si = */ false);

        for (const auto& value : solution_keys) {
            const auto& vector = value.key;
            const auto* kw     = rst_view.getKeyword(vector.c_str());

            if (kw == nullptr) {
                throwIfMissingRequired(value);

                // Requested vector not available, but caller does not
                // actually require the vector for restart purposes.
                // Skip this.
                continue;
            }

            if (Opm::RestartIO::ecl_kw_get_size(kw) != numcells) {
                throw std::runtime_error {
                    "Restart file: Could not restore "
                    + std::string(Opm::RestartIO::ecl_kw_get_header(kw))
                    + ", mismatched number of cells"
                };
            }

            sol.insert(vector, value.dim, double_vector(kw),
                       Opm::data::TargetType::RESTART_SOLUTION);
        }

        return sol;
    }

    void restoreExtra(const RestartFileView&              rst_view,
                      const std::vector<Opm::RestartKey>& extra_keys,
                      const Opm::UnitSystem&              usys,
                      Opm::RestartValue&                  rst_value)
    {
        for (const auto& extra : extra_keys) {
            const auto& vector = extra.key;
            const auto* kw     = rst_view.getKeyword(vector.c_str());

            auto kwdata = std::vector<double>{};

            if (kw == nullptr) {
                // Requested vector not available in result set.  Take
                // appropriate action depending on specific vector and
                // 'extra.required'.

                if (vector != "OPMEXTRA") {
                    throwIfMissingRequired(extra);

                    // Requested vector not available, but caller does not
                    // actually require the vector for restart purposes.
                    // Skip this.
                    continue;
                }
                else {
                    // Special case handling of OPMEXTRA.  Single item
                    // possibly stored in TSINIT item of DOUBHEAD.  Try to
                    // recover this.  Function throws if item is defaulted
                    // and caller requires that item be present through the
                    // 'extra.required' mechanism.

                    kwdata = getOpmExtraFromDoubHEAD(rst_view,
                                                     extra.required,
                                                     usys);
                }
            }
            else {
                // Requisite vector available in result set.  Recover data.
                kwdata = double_vector(kw);
            }

            rst_value.addExtra(vector, extra.dim, std::move(kwdata));
        }

        for (auto& extra_value : rst_value.extra) {
            const auto& restart_key = extra_value.first;
            auto&       data        = extra_value.second;

            usys.to_si(restart_key.dim, data);
        }
    }

    void checkWellVectorSizes(const ::Opm::RestartIO::ecl_kw_type*      opm_xwel,
                              const ::Opm::RestartIO::ecl_kw_type*      opm_iwel,
                              const int                                 sim_step,
                              const std::vector<Opm::data::Rates::opt>& phases,
                              const std::vector<const Opm::Well*>&      sched_wells)
    {
        const auto expected_xwel_size =
            std::accumulate(sched_wells.begin(), sched_wells.end(),
                            std::size_t(0),
                [&phases, sim_step](const std::size_t acc, const Opm::Well* w)
                -> std::size_t
            {
                return acc
                    + 2 + phases.size()
                    + (w->getConnections(sim_step).size()
                        * (phases.size() + Opm::data::Connection::restart_size));
            });

        if (static_cast<std::size_t>(::Opm::RestartIO::ecl_kw_get_size(opm_xwel)) != expected_xwel_size)
        {
            throw std::runtime_error {
                "Mismatch between OPM_XWEL and deck; "
                "OPM_XWEL size was " + std::to_string(::Opm::RestartIO::ecl_kw_get_size(opm_xwel)) +
                ", expected " + std::to_string(expected_xwel_size)
            };
        }

        if (::Opm::RestartIO::ecl_kw_get_size(opm_iwel) != int(sched_wells.size())) {
            throw std::runtime_error {
                "Mismatch between OPM_IWEL and deck; "
                "OPM_IWEL size was " + std::to_string(::Opm::RestartIO::ecl_kw_get_size(opm_iwel)) +
                ", expected " + std::to_string(sched_wells.size())
            };
        }
    }

    Opm::data::Wells
    restore_wells_opm(const RestartFileView&     rst_view,
                      const ::Opm::EclipseState& es,
                      const ::Opm::EclipseGrid&  grid,
                      const ::Opm::Schedule&     schedule)
    {
        namespace Load = ::Opm::RestartIO;

        const auto* opm_iwel = rst_view.getKeyword("OPM_IWEL");
        const auto* opm_xwel = rst_view.getKeyword("OPM_XWEL");

        if ((opm_xwel == nullptr) || (opm_iwel == nullptr)) {
            return {};
        }

        using rt = Opm::data::Rates::opt;

        const auto& sched_wells = schedule.getWells(rst_view.simStep());
        std::vector<rt> phases;
        {
            const auto& phase = es.runspec().phases();

            if (phase.active(Opm::Phase::WATER)) { phases.push_back(rt::wat); }
            if (phase.active(Opm::Phase::OIL))   { phases.push_back(rt::oil); }
            if (phase.active(Opm::Phase::GAS))   { phases.push_back(rt::gas); }
        }

        checkWellVectorSizes(opm_xwel, opm_iwel,
                             rst_view.simStep(),
                             phases, sched_wells);

        Opm::data::Wells wells;
        const auto* opm_xwel_data = Load::ecl_kw_get_type_ptr<double>(opm_xwel, Load::ECL_DOUBLE_TYPE);
        const auto* opm_iwel_data = Load::ecl_kw_get_type_ptr<int>(opm_iwel, Load::ECL_INT_TYPE);

        for (const auto* sched_well : sched_wells) {
            auto& well = wells[ sched_well->name() ];

            well.bhp         = *opm_xwel_data++;
            well.temperature = *opm_xwel_data++;
            well.control     = *opm_iwel_data++;

            for (const auto& phase : phases) {
                well.rates.set(phase, *opm_xwel_data++);
            }

            for (const auto& sc : sched_well->getConnections(rst_view.simStep())) {
                const auto i = sc.getI(), j = sc.getJ(), k = sc.getK();

                if (!grid.cellActive(i, j, k) || sc.state() == Opm::WellCompletion::SHUT) {
                    opm_xwel_data += Opm::data::Connection::restart_size + phases.size();
                    continue;
                }

                well.connections.emplace_back();
                auto& connection = well.connections.back();

                connection.index          = grid.getGlobalIndex(i, j, k);
                connection.pressure       = *opm_xwel_data++;
                connection.reservoir_rate = *opm_xwel_data++;

                for (const auto& phase : phases) {
                    connection.rates.set(phase, *opm_xwel_data++);
                }
            }
        }

        return wells;
    }

    std::map<
        std::tuple<int, int, int>,
        boost::iterator_range<const double*>::size_type
    >
    ijk_to_resID(const std::size_t  wellID,
                 const std::size_t  nConn,
                 const WellVectors& wellData)
    {
        using SizeT    = boost::iterator_range<const double*>::size_type;
        auto  ijkToRes = std::map<std::tuple<int, int, int>, SizeT>{};

        for (auto connID = 0*nConn; connID < nConn; ++connID) {
            const auto icon = wellData.icon(wellID, connID);

            const auto i = icon[VI::IConn::index::CellI] - 1;
            const auto j = icon[VI::IConn::index::CellJ] - 1;
            const auto k = icon[VI::IConn::index::CellK] - 1;

            ijkToRes.emplace(std::make_tuple(i, j, k), connID);
        }

        return ijkToRes;
    }

    void restoreConnRates(const WellVectors::Window<double>& xcon,
                          const Opm::UnitSystem&             usys,
                          const bool                         oil,
                          const bool                         gas,
                          const bool                         wat,
                          Opm::data::Connection&             xc)
    {
        using M = ::Opm::UnitSystem::measure;

        if (wat) {
            xc.rates.set(Opm::data::Rates::opt::wat,
                         - usys.to_si(M::liquid_surface_rate,
                                      xcon[VI::XConn::index::WaterRate]));
        }

        if (oil) {
            xc.rates.set(Opm::data::Rates::opt::oil,
                         - usys.to_si(M::liquid_surface_rate,
                                      xcon[VI::XConn::index::OilRate]));
        }

        if (gas) {
            xc.rates.set(Opm::data::Rates::opt::gas,
                         - usys.to_si(M::gas_surface_rate,
                                      xcon[VI::XConn::index::GasRate]));
        }
    }

    void zeroConnRates(const bool             oil,
                       const bool             gas,
                       const bool             wat,
                       Opm::data::Connection& xc)
    {
        if (wat) { xc.rates.set(Opm::data::Rates::opt::wat, 0.0); }
        if (oil) { xc.rates.set(Opm::data::Rates::opt::oil, 0.0); }
        if (gas) { xc.rates.set(Opm::data::Rates::opt::gas, 0.0); }
    }

    void restoreConnResults(const Opm::Well&        well,
                            const std::size_t       wellID,
                            const std::size_t       sim_step,
                            const Opm::EclipseGrid& grid,
                            const Opm::UnitSystem&  usys,
                            const Opm::Phases&      phases,
                            const WellVectors&      wellData,
                            Opm::data::Well&        xw)
    {
        using M  = ::Opm::UnitSystem::measure;
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XConn::index;

        const auto iwel  = wellData.iwel(wellID);
        const auto nConn = static_cast<std::size_t>(
            iwel[VI::IWell::index::NConn]);

        xw.connections.resize(nConn, Opm::data::Connection{});

        const auto oil = phases.active(Opm::Phase::OIL);
        const auto gas = phases.active(Opm::Phase::GAS);
        const auto wat = phases.active(Opm::Phase::WATER);

        for (auto& xc : xw.connections) {
            zeroConnRates(oil, gas, wat, xc);
        }

        if (! wellData.hasDefinedConnectionValues()) {
            // Result set does not provide certain pieces of information
            // which are needed to reconstruct connection flow rates.
            // Nothing to do except to return all zero rates.
            return;
        }

        const auto conns      = well.getActiveConnections(sim_step, grid);
        const auto ijk_to_res = ijk_to_resID(wellID, nConn, wellData);

        auto linConnID = std::size_t{0};
        for (const auto& conn : conns) {
            if (++linConnID > nConn) { continue; }

            auto& xc = xw.connections[linConnID - 1];

            const auto ijk =
                std::make_tuple(conn.getI(), conn.getJ(), conn.getK());

            auto resPos = ijk_to_res.find(ijk);

            if (resPos != std::end(ijk_to_res)) {
                const auto connID = resPos->second;
                const auto xcon   = wellData.xcon(wellID, connID);

                restoreConnRates(xcon, usys, oil, gas, wat, xc);

                xc.index = grid.getGlobalIndex(std::get<0>(ijk),
                                               std::get<1>(ijk),
                                               std::get<2>(ijk));

                xc.pressure = usys.to_si(M::pressure, xcon[Ix::Pressure]);
            }
        }
    }

    void restoreSegmentQuantities(const std::size_t      mswID,
                                  const std::size_t      numSeg,
                                  const Opm::UnitSystem& usys,
                                  const Opm::Phases&     phases,
                                  const SegmentVectors&  segData,
                                  Opm::data::Well&       xw)
    {
        // Note sign of flow rates.  RSEG stores flow rates as positive from
        // reservoir to well--i.e., towards the producer/platform.  The Flow
        // simulator uses the opposite sign convention.

        using M = ::Opm::UnitSystem::measure;

        const auto oil = phases.active(Opm::Phase::OIL);
        const auto gas = phases.active(Opm::Phase::GAS);
        const auto wat = phases.active(Opm::Phase::WATER);

        // Renormalisation constant for gas okay in non-field unit systems.
        // A bit more questionable for field units.
        const auto watRenormalisation =   10.0;
        const auto gasRenormalisation = 1000.0;

        for (auto segID = 0*numSeg; segID < numSeg; ++segID) {
            const auto iseg = segData.iseg(mswID, segID);
            const auto rseg = segData.rseg(mswID, segID);

            const auto segNumber = iseg[VI::ISeg::index::SegNo]; // One-based

            auto& segment = xw.segments[segNumber];

            segment.segNumber = segNumber;
            segment.pressure  =
                usys.to_si(M::pressure, rseg[VI::RSeg::index::Pressure]);

            const auto totFlow     = rseg[VI::RSeg::index::TotFlowRate];
            const auto watFraction = rseg[VI::RSeg::index::WatFlowFract];
            const auto gasFraction = rseg[VI::RSeg::index::GasFlowFract];

            if (wat) {
                const auto qW = totFlow * watFraction * watRenormalisation;
                segment.rates.set(Opm::data::Rates::opt::wat,
                                  - usys.to_si(M::liquid_surface_rate, qW));
            }

            if (oil) {
                const auto qO = totFlow * (1.0 - (watFraction + gasFraction));
                segment.rates.set(Opm::data::Rates::opt::oil,
                                  - usys.to_si(M::liquid_surface_rate, qO));
            }

            if (gas) {
                const auto qG = totFlow * gasFraction * gasRenormalisation;
                segment.rates.set(Opm::data::Rates::opt::gas,
                                  - usys.to_si(M::gas_surface_rate, qG));
            }
        }
    }

    Opm::data::Well
    restore_well(const Opm::Well&        well,
                 const std::size_t       wellID,
                 const std::size_t       sim_step,
                 const Opm::EclipseGrid& grid,
                 const Opm::UnitSystem&  usys,
                 const Opm::Phases&      phases,
                 const WellVectors&      wellData,
                 const SegmentVectors&   segData)
    {
        if (! wellData.hasDefinedWellValues()) {
            // Result set does not provide well information.
            // No wells?  In any case, nothing to do here.
            return {};
        }

        using M = ::Opm::UnitSystem::measure;

        const auto xwel = wellData.xwel(wellID);

        const auto oil = phases.active(Opm::Phase::OIL);
        const auto gas = phases.active(Opm::Phase::GAS);
        const auto wat = phases.active(Opm::Phase::WATER);

        auto xw = ::Opm::data::Well{};

        // 1) Restore well rates (xw.rates)
        if (wat) {
            xw.rates.set(Opm::data::Rates::opt::wat,
                         - usys.to_si(M::liquid_surface_rate,
                                      xwel[VI::XWell::index::WatPrRate]));
        }

        if (oil) {
            xw.rates.set(Opm::data::Rates::opt::oil,
                         - usys.to_si(M::liquid_surface_rate,
                                      xwel[VI::XWell::index::OilPrRate]));
        }

        if (gas) {
            xw.rates.set(Opm::data::Rates::opt::gas,
                         - usys.to_si(M::gas_surface_rate,
                                      xwel[VI::XWell::index::GasPrRate]));
        }

        // 2) Restore other well quantities (really only xw.bhp)
        xw.bhp = usys.to_si(M::pressure, xwel[VI::XWell::index::FlowBHP]);
        xw.thp = xw.temperature = 0.0;

        // 3) Restore connection flow rates (xw.connections[i].rates)
        //    and pressure values (xw.connections[i].pressure).
        restoreConnResults(well, wellID, sim_step,
                           grid, usys, phases, wellData, xw);

        // 4) Restore segment quantities if applicable.
        if (well.isMultiSegment(sim_step) &&
            segData.hasDefinedValues())
        {
            const auto iwel   = wellData.iwel(wellID);
            const auto mswID  = iwel[VI::IWell::index::MsWID]; // One-based
            const auto numSeg = iwel[VI::IWell::index::NWseg];

            if ((mswID > 0) && (numSeg > 0)) {
                restoreSegmentQuantities(mswID - 1, numSeg, usys,
                                         phases, segData, xw);
            }
        }

        return xw;
    }

    Opm::data::Wells
    restore_wells_ecl(const RestartFileView&     rst_view,
                      const ::Opm::EclipseState& es,
                      const ::Opm::EclipseGrid&  grid,
                      const ::Opm::Schedule&     schedule)
    {
        auto soln = ::Opm::data::Wells{};

        const auto* intehead = rst_view.getKeyword("INTEHEAD");

        if (intehead == nullptr) {
            // Result set does not provide indexing information.
            // Can't do anything here.
            return soln;
        }

        const auto wellData = WellVectors   { rst_view, intehead };
        const auto segData  = SegmentVectors{ rst_view, intehead };

        const auto& units  = es.getUnits();
        const auto& phases = es.runspec().phases();

        const auto  sim_step = rst_view.simStep();
        const auto& wells    = schedule.getWells(sim_step);
        for (auto nWells = wells.size(), wellID = 0*nWells;
             wellID < nWells; ++wellID)
        {
            const auto* well = wells[wellID];

            soln[well->name()] =
                restore_well(*well, wellID, sim_step, grid,
                             units, phases, wellData, segData);
        }

        return soln;
    }

    void assign_well_cumulatives(const std::string& well,
                                 const std::size_t  wellID,
                                 const WellVectors& wellData,
                                 Opm::SummaryState& smry)
    {
        if (! wellData.hasDefinedWellValues()) {
            // Result set does not provide well information.
            // No wells?  In any case, nothing to do here.
            return;
        }

        auto key = [&well](const std::string& vector) -> std::string
        {
            return vector + ':' + well;
        };

        const auto xwel = wellData.xwel(wellID);

        // No unit conversion here.  Smry expects output units.
        smry.add(key("WOPT"), xwel[VI::XWell::index::OilPrTotal]);
        smry.add(key("WWPT"), xwel[VI::XWell::index::WatPrTotal]);
        smry.add(key("WGPT"), xwel[VI::XWell::index::GasPrTotal]);
        smry.add(key("WVPT"), xwel[VI::XWell::index::VoidPrTotal]);

        smry.add(key("WWIT"), xwel[VI::XWell::index::WatInjTotal]);
        smry.add(key("WGIT"), xwel[VI::XWell::index::GasInjTotal]);
    }

    void assign_group_cumulatives(const std::string&  group,
                                  const std::size_t   groupID,
                                  const GroupVectors& groupData,
                                  Opm::SummaryState&  smry)
    {
        if (! groupData.hasDefinedValues()) {
            // Result set does not provide group information.
            // No wells?  In any case, nothing to do here.
            return;
        }

        auto key = [&group](const std::string& vector) -> std::string
        {
            return (group == "FIELD")
                ?  'F' + vector
                :  'G' + vector + ':' + group;
        };

        const auto xgrp = groupData.xgrp(groupID);

        // No unit conversion here.  Smry expects output units.
        smry.add(key("OPT"), xgrp[VI::XGroup::index::OilPrTotal]);
        smry.add(key("WPT"), xgrp[VI::XGroup::index::WatPrTotal]);
        smry.add(key("GPT"), xgrp[VI::XGroup::index::GasPrTotal]);
        smry.add(key("VPT"), xgrp[VI::XGroup::index::VoidPrTotal]);

        smry.add(key("WIT"), xgrp[VI::XGroup::index::WatInjTotal]);
        smry.add(key("GIT"), xgrp[VI::XGroup::index::GasInjTotal]);
    }

    Opm::SummaryState
    restore_cumulative(const RestartFileView& rst_view,
                       const ::Opm::Schedule& schedule)
    {
        auto smry = Opm::SummaryState{};

        const auto  sim_step = rst_view.simStep();
        const auto* intehead = rst_view.getKeyword("INTEHEAD");

        if (intehead == nullptr) { return smry; }

        // Well cumulatives
        {
            const auto  wellData = WellVectors { rst_view, intehead };
            const auto& wells    = schedule.getWells(sim_step);

            for (auto nWells = wells.size(), wellID = 0*nWells;
                 wellID < nWells; ++wellID)
            {
                assign_well_cumulatives(wells[wellID]->name(),
                                        wellID, wellData, smry);
            }
        }

        // Group cumulatives, including FIELD.
        {
            const auto groupData = GroupVectors { rst_view, intehead };

            for (const auto* group : schedule.getGroups(sim_step)) {
                const auto& gname = group->name();

                // Note: Order of group values in {I,X}GRP arrays mostly
                // matches group's order of occurrence in .DATA file.
                // Values pertaining to FIELD are stored at zero-based order
                // index NGMAXZ (maximum number of groups in model).  The
                // latter value is groupData.maxGroups().
                //
                // As a final wrinkle, Flow internally stores FIELD at
                // seqIndex() == 0, so subtract one to account for this
                // fact.  Max(seqIndex(), 1) - 1 is just a bit of future
                // proofing and robustness in case that ever changes.
                const auto groupOrderIx = (gname == "FIELD")
                    ? groupData.maxGroups() // NGMAXZ -- Item 3 of WELLDIMS
                    : std::max(group->seqIndex(), std::size_t{1}) - 1;

                assign_group_cumulatives(gname, groupOrderIx, groupData, smry);
            }
        }

        return smry;
    }
} // Anonymous namespace

namespace Opm { namespace RestartIO  {

    std::pair<RestartValue, SummaryState>
    load(const std::string&             filename,
         int                            report_step,
         const std::vector<RestartKey>& solution_keys,
         const EclipseState&            es,
         const EclipseGrid&             grid,
         const Schedule&                schedule,
         const std::vector<RestartKey>& extra_keys)
    {
        const auto rst_view = RestartFileView{ filename, report_step };

        auto xr = restoreSOLUTION(rst_view, solution_keys,
                                  grid.getNumActive());

        xr.convertToSI(es.getUnits());

        auto xw = Opm::RestartIO::ecl_file_view_has_kw(rst_view, "OPM_XWEL")
            ? restore_wells_opm(rst_view, es, grid, schedule)
            : restore_wells_ecl(rst_view, es, grid, schedule);

        auto rst_value = RestartValue{ std::move(xr), std::move(xw) };

        if (! extra_keys.empty()) {
            restoreExtra(rst_view, extra_keys, es.getUnits(), rst_value);
        }

        return {
            std::move(rst_value),
            restore_cumulative(rst_view, schedule)
        };
    }

}} // Opm::RestartIO
