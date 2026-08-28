/*
  Copyright (c) 2021 Equinor ASA
  Copyright (c) 2018 Statoil ASA

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

#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <opm/output/eclipse/InteHEAD.hpp>
#include <opm/output/eclipse/VectorItems/connection.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>
#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Regdims.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>
#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/ArrayDimChecker.hpp>
#include <opm/input/eclipse/Schedule/GasLiftOpt.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateConfig.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateModel.hpp>
#include <opm/input/eclipse/Schedule/Network/Balance.hpp>
#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Tuning.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQActive.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQInput.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

    struct WellArrayDims
    {
        /// Number of entries per well in the IWEL array.
        int iwell{};

        /// Number of entries per well in the SWEL array.
        int swell{};

        /// Number of entries per well in the XWEL array.
        int xwell{};

        /// Number of entries per well in the ZWEL array.
        int zwell{3};
    };

    struct ConnArrayDims
    {
        /// Number of entries per connection in the ICON array.
        int iconn{};

        /// Number of entries per connection in the SCON array.
        int sconn{};

        /// Number of entries per connection in the XCON array.
        int xconn{};
    };

    // Declared number of tracers in the run.  Temperature, if present,
    // is treated as a tracer for the purpose of determining the number
    // of well array elements.  Oil and gas tracers have both free and
    // solution parts, so they count double for the number of elements.
    struct TracerSizes
    {
        explicit TracerSizes(const Opm::Runspec&          rspec,
                             const Opm::SimulationConfig& simConfig)
            : numTracers(rspec.tracers().water_tracers()
                         + rspec.tracers().oil_tracers()
                         + rspec.tracers().gas_tracers()
                         + (rspec.temp() ? 1 : 0))
            , numTracerElems(numTracers)
        {
            if (simConfig.hasDISGAS()) {
                // Gas tracers have both free and solution parts when
                // gas dissolves in the oil phase.  Include solution
                // part in the count of array elements.
                numTracerElems += rspec.tracers().gas_tracers();
            }

            if (simConfig.hasVAPOIL()) {
                // Oil tracers have both free and solution parts when
                // oil vaporizes into the gas phase.  Include solution
                // part in the count of array elements.
                numTracerElems += rspec.tracers().oil_tracers();
            }
        }

        // Maximum number of tracer-like objects in the run,
        // including temperature if present.
        int numTracers{};

        // Declared number of array elements per tracer quantity
        // in the run.
        int numTracerElems{};
    };

    using nph_enum = Opm::GuideRateModel::Target;
    const std::map<nph_enum, int> nph_enumToECL = {
        {nph_enum::NONE, 0},
        {nph_enum::OIL,  1},
        {nph_enum::GAS,  3},
        {nph_enum::LIQ,  4},
        {nph_enum::RES,  6},
        {nph_enum::COMB, 9},
    };

    using prod_cmode = Opm::Well::ProducerCMode;
    const std::map<prod_cmode, int> prod_cmodeToECL = {
        {prod_cmode::NONE,  0},
        {prod_cmode::ORAT,  1},
        {prod_cmode::WRAT,  2},
        {prod_cmode::GRAT,  3},
        {prod_cmode::LRAT,  4},
        {prod_cmode::RESV,  5},
        {prod_cmode::BHP,   7},
    };

    int maxConnPerWell(const Opm::Schedule& sched,
                       const std::size_t    report_step,
                       const std::size_t    lookup_step)
    {
        if (report_step == std::size_t{0}) {
            return 0;
        }

        auto ncwmax = 0;
        for (const auto& well : sched.getWells(lookup_step)) {
            const auto ncw = well.getConnections().size();

            ncwmax = std::max(ncwmax, static_cast<int>(ncw));
        }

        return ncwmax;
    }

    int numGroupsInField(const Opm::Schedule& sched,
                         const std::size_t    lookup_step)
    {
        const auto ngmax = sched[lookup_step].groups.size();

        if (ngmax < 1) {
            throw std::invalid_argument {
                "Simulation run must include at least FIELD group"
            };
        }

        // Number of non-FIELD groups.
        return ngmax - 1;
    }

    int numGroupsInField(const Opm::Schedule& sched,
                         const std::size_t    lookup_step,
                         [[maybe_unused]] const std::string&   lgr_tag)
    {
        return numGroupsInField(sched, lookup_step);
        // Following code should be enabled when AggregateGroupData.cpp is fixed
        // if (lgr_tag == "GLOBAL" or  lgr_tag.empty()){
        //     return numGroupsInField(sched, lookup_step);
        // }
        // else {
        //     // This is the default value and correspond to a single well group for LGR grids.
        //     return 1;
        // }
    }

    int GroupControl(const Opm::Schedule& sched,
                     const std::size_t    report_step,
                     const std::size_t    lookup_step)
    {
        int gctrl = 0;
        if (report_step == std::size_t{0}) {
            return gctrl;
        }
        bool have_gconprod = false;
        bool have_gconinje = false;

        for (const auto& group_name : sched.groupNames(lookup_step)) {
            const auto& group = sched.getGroup(group_name, lookup_step);
            if (group.isProductionGroup()) {
                have_gconprod = true;
            }
            if (group.isInjectionGroup()) {
                have_gconinje = true;
            }
        }
        if (have_gconinje)
            gctrl = 2;
        else if (have_gconprod)
            gctrl = 1;

        // Index for group control
        return gctrl;
    }

    int noIuads(const Opm::Schedule& sched,
                const std::size_t    rptStep,
                const std::size_t    simStep)
    {
        if (rptStep == std::size_t{0}) {
            return 0;
        }

        return static_cast<int>
            (sched[simStep].udq_active().iuad().size());
    }

    int noIuaps(const Opm::Schedule& sched,
                const std::size_t    rptStep,
                const std::size_t    simStep)
    {
        if (rptStep == std::size_t{0}) {
            return 0;
        }

        // UDQActive::iuap() returns a vector<> by value.
        const auto iuap = sched[simStep].udq_active().iuap();

        return std::accumulate(iuap.begin(), iuap.end(), 0,
            [](const int n, const auto& rec)
        {
            switch (Opm::UDQ::keyword(rec.control)) {
            case Opm::UDAKeyword::GCONINJE:
                // Group level injection UDA
                return n + 3;

            case Opm::UDAKeyword::GCONPROD:
                // Group level production UDA
                return n + 2;

            default:
                // Well level UDA.
                return n + 1;
            }
        });
    }

    int numMultiSegWells(const ::Opm::Schedule& sched,
                         const std::size_t      report_step,
                         const std::size_t      lookup_step)
    {
        if (report_step == 0) { return 0; }

        const auto& wnames = sched.wellNames(lookup_step);

        return std::ranges::count_if(wnames,
                                    [&sched, lookup_step](const std::string& wname) -> bool
                                    { return sched.getWell(wname, lookup_step).isMultiSegment(); });
    }

    int maxNumSegments(const ::Opm::Schedule& sched,
                       const std::size_t      report_step,
                       const std::size_t      lookup_step)
    {
        if (report_step == 0) { return 0; }

        const auto& wnames = sched.wellNames(lookup_step);

        return std::accumulate(std::begin(wnames), std::end(wnames), 0,
            [&sched, lookup_step](const int m, const std::string& wname) -> int
        {
            // maxSegmentID() returns 0 for standard (non-MS) wells.
            return std::max(m, sched.getWell(wname, lookup_step).maxSegmentID());
        });
    }

    int maxNumLateralBranches(const ::Opm::Schedule& sched,
                              const std::size_t      report_step,
                              const std::size_t      lookup_step)
    {
        if (report_step == 0) { return 0; }

        const auto& wnames = sched.wellNames(lookup_step);

        return std::accumulate(std::begin(wnames), std::end(wnames), 0,
            [&sched, lookup_step](const int m, const std::string& wname) -> int
        {
            // maxBranchID() returns 0 for standard (non-MS) wells.
            return std::max(m, sched.getWell(wname, lookup_step).maxBranchID());
        });
    }

    Opm::RestartIO::InteHEAD::WellTableDim
    getWellTableDims(const int              nwgmax,
                     const int              ngmax,
                     const ::Opm::Runspec&  rspec,
                     const ::Opm::Schedule& sched,
                     const std::size_t      report_step,
                     const std::size_t      lookup_step)
    {
        const auto& wd = rspec.wellDimensions();

        const auto numWells = static_cast<int>(sched.numWells(lookup_step));

        const auto maxPerf =
            std::max(wd.maxConnPerWell(),
                     maxConnPerWell(sched, report_step, lookup_step));

        const auto maxWellInGroup =
            std::max(wd.maxWellsPerGroup(), nwgmax);

        const auto maxGroupInField =
            std::max(wd.maxGroupsInField(), ngmax);

        const auto nWMaxz = wd.maxWellsInField();

        return {
            (report_step > 0) ? numWells : 0,
            maxPerf,
            maxWellInGroup,
            maxGroupInField,
            (report_step > 0) ? std::max(nWMaxz, numWells) : nWMaxz,
            wd.maxWellListsPrWell(),
            wd.maxDynamicWellLists()
        };
    }

    Opm::RestartIO::InteHEAD::WellTableDim
    getWellTableDims(const int              nwgmax,
                     const int              ngmax,
                     const ::Opm::Runspec&  rspec,
                     const ::Opm::Schedule& sched,
                     const std::size_t      report_step,
                     const std::size_t      lookup_step,
                     const std::string&     lgr_tag)
    {
        if ((lgr_tag == "GLOBAL") or (lgr_tag.empty()))
        {
            return getWellTableDims(nwgmax,ngmax,rspec,sched,report_step,lookup_step);
        }

        const auto& wd = rspec.wellDimensions();

        const auto schedule_state = sched[lookup_step];
        const auto wnames = sched.wellNames(lookup_step);
        int numWells =
            std::ranges::count_if(wnames,
                                  [&lgr_tag, &sched = sched[lookup_step]](const auto& wname)
                                  { return sched.wells(wname).get_lgr_well_tag().value_or("") == lgr_tag; });

        const auto maxPerf =
            std::max(wd.maxConnPerWell(),
                     maxConnPerWell(sched, report_step, lookup_step));

        const auto maxWellInGroup =
             std::max(wd.maxWellsPerGroup(), nwgmax); // WellsPerGroup computed in terms of the Global Grid

        // This seems to be some sort of default value for LGR grid and should be enabled when AggregateGroupData.cpp is fixed.
        const auto maxGroupInField = 1;

        const auto nWMaxz = wd.maxWellsInField();

        return {
            (report_step > 0) ? numWells : 0,
            maxPerf,
            maxWellInGroup,
            maxGroupInField,
            (report_step > 0) ? std::max(nWMaxz, numWells) : nWMaxz,
            wd.maxWellListsPrWell(),
            wd.maxDynamicWellLists()
        };
    }

    WellArrayDims getWellArrayDims(const TracerSizes& tz)
    {
        namespace VectorItems = Opm::RestartIO::Helpers::VectorItems;

        // Five quantities (flow rate, cumulative production, cumulative injection, 2*concentration)
        // per tracer component, plus two additional elements if any tracers are present.
        constexpr auto tracer_quantity_count = 5;
        const auto nxwelz_tracer_elems =
            tracer_quantity_count*tz.numTracerElems
            + ((tz.numTracers > 0) ? 2 : 0);

        constexpr auto iwTo = static_cast<int>(VectorItems::IWell::index::TracerOffset);
        constexpr auto swTo = static_cast<int>(VectorItems::SWell::index::TracerOffset);
        constexpr auto xwTo = static_cast<int>(VectorItems::XWell::index::TracerOffset);

        return {
            .iwell = iwTo + tz.numTracers,         // #IWEL elems per well
            .swell = swTo + 2 * tz.numTracerElems, // #SWEL elems per well
            .xwell = xwTo + nxwelz_tracer_elems,   // #XWEL elems per well
            .zwell = 3                             // #ZWEL elems per well
        };
    }

    ConnArrayDims getConnArrayDims(const TracerSizes& tz)
    {
        namespace VectorItems = Opm::RestartIO::Helpers::VectorItems;

        // Allocate XCON space for five quantities per tracer component.
        constexpr auto tracer_quantity_count = 5;
        const auto nxcon_tracer_elems = tracer_quantity_count * tz.numTracerElems;

        constexpr auto xcTo = static_cast<int>(VectorItems::XConn::index::TracerOffset);

        return {
            .iconn = 26,                          // #ICON elems per conn
            .sconn = 42,                          // #SCON elems per conn
            .xconn = xcTo + nxcon_tracer_elems    // #XCON elems per conn
        };
    }

    std::array<int, 4>
    getNGRPZ(const ::Opm::Runspec& rspec,
             const TracerSizes&    tz,
             const int             grpsz,
             const int             ngrp)
    {
        namespace VectorItems = Opm::RestartIO::Helpers::VectorItems;

        const auto& wd = rspec.wellDimensions();

        const auto nwgmax = std::max(grpsz, wd.maxWellsPerGroup());
        const auto ngmax  = std::max(ngrp , wd.maxGroupsInField());

        const int nigrpz = 97 + std::max(nwgmax, ngmax);
        const int nsgrpz = 112;
        const int nxgrpz = static_cast<int>(VectorItems::XGroup::index::TracerOffset) + 4*tz.numTracerElems;
        const int nzgrpz = 5;

        return {
            nigrpz,
            nsgrpz,
            nxgrpz,
            nzgrpz,
        };
    }

    Opm::RestartIO::InteHEAD::Phases
    getActivePhases(const ::Opm::Runspec& rspec)
    {
        auto phases = ::Opm::RestartIO::InteHEAD::Phases{};

        const auto& phasePred = rspec.phases();

        phases.oil   = phasePred.active(Opm::Phase::OIL);
        phases.water = phasePred.active(Opm::Phase::WATER);
        phases.gas   = phasePred.active(Opm::Phase::GAS);

        return phases;
    }

    Opm::RestartIO::InteHEAD::TuningPar
    getTuningPars(const ::Opm::Tuning& tuning)
    {
        return {
            tuning.NEWTMX,
            tuning.NEWTMN,
            tuning.LITMAX,
            tuning.LITMIN,
            tuning.MXWSIT,
            tuning.MXWPIT,
            tuning.WSEG_MAX_RESTART
        };
    }

    Opm::RestartIO::InteHEAD::UdqParam
    getUdqParam(const ::Opm::Runspec& rspec,
                const Opm::Schedule&  sched,
                const std::size_t     rptStep,
                const std::size_t     simStep)
    {
        auto param = Opm::RestartIO::InteHEAD::UdqParam{};

        if (rptStep == std::size_t{0}) {
            return param;
        }

        param.udqParam_1 = rspec.udqParams().rand_seed();

        param.num_iuads = noIuads(sched, rptStep, simStep);
        param.num_iuaps = noIuaps(sched, rptStep, simStep);

        sched[simStep].udq().exportTypeCount(param.numUDQs);

        return param;
    }

    Opm::RestartIO::InteHEAD::ActionParam
    getActionParam(const ::Opm::Runspec&         rspec,
                   const ::Opm::Action::Actions& acts,
                   const std::size_t             rptStep)
    {
        if (rptStep == std::size_t{0}) {
            return { 0, 0, 0, 0 };
        }

        const auto no_act = acts.ecl_size();
        const auto max_lines_pr_action = acts.max_input_lines();
        const auto max_cond_per_action = rspec.actdims().max_conditions();
        const auto max_characters_per_line = rspec.actdims().max_characters();

        return {
            static_cast<int>(no_act),
            max_lines_pr_action,
            static_cast<int>(max_cond_per_action),
            static_cast<int>(max_characters_per_line)
        };
    }

    Opm::RestartIO::InteHEAD::WellSegDims
    getWellSegDims(const ::Opm::Runspec&  rspec,
                   const ::Opm::Schedule& sched,
                   const TracerSizes&     tz,
                   const std::size_t      report_step,
                   const std::size_t      lookup_step)
    {
        const auto& wsd = rspec.wellSegmentDimensions();

        const auto numMSW = numMultiSegWells(sched, report_step, lookup_step);
        const auto maxNumSeg = maxNumSegments(sched, report_step, lookup_step);
        const auto maxNumBr = maxNumLateralBranches(sched, report_step, lookup_step);

        return {
            .nsegwl = numMSW,
            .nswlmx = std::max(numMSW, wsd.maxSegmentedWells()),
            .nsegmx = std::max(maxNumSeg, wsd.maxSegmentsPerWell()),
            .nlbrmx = std::max(maxNumBr, wsd.maxLateralBranchesPerWell()),
            .nisegz = 22,                       // #ISEG elems per segment
            .nrsegz = Opm::RestartIO::InteHEAD::numRsegElem(rspec.phases())
               + 8*tz.numTracerElems,           // #RSEG elems per segment
            .nilbrz = 10                        // #ILBR elems per branch
        };
    }

    Opm::RestartIO::InteHEAD::RegDims
    getRegDims(const ::Opm::TableManager& tdims,
               const ::Opm::Regdims&      rdims)
    {
        const auto ntfip  = tdims.numFIPRegions();
        const auto nmfipr = rdims.getNMFIPR();
        const auto nrfreg = rdims.getNRFREG();
        const auto ntfreg = rdims.getNTFREG();
        const auto nplmix = rdims.getNPLMIX();

        return {
            static_cast<int>(ntfip),
            static_cast<int>(nmfipr),
            static_cast<int>(nrfreg),
            static_cast<int>(ntfreg),
            static_cast<int>(nplmix),
        };
    }

    Opm::RestartIO::InteHEAD::RockOpts
    getRockOpts(const ::Opm::RockConfig& rckCfg, const Opm::Regdims& reg_dims)
    {
        int nttyp  = 1;   // Default value (PVTNUM)
        if (rckCfg.rocknum_property() == "SATNUM") nttyp = 2;
        if (rckCfg.rocknum_property() == "ROCKNUM") nttyp = 4 + reg_dims.getNMFIPR();

        return {
            nttyp
        };
    }

    Opm::RestartIO::InteHEAD::GuideRateNominatedPhase
    setGuideRateNominatedPhase(const ::Opm::Schedule& sched,
                               const std::size_t      report_step,
                               const std::size_t      lookup_step)
    {
        int nom_phase = 0;
        if (report_step == std::size_t{0}) {
            return { nom_phase };
        }

        const auto& guideCFG = sched[lookup_step].guide_rate();
        if (guideCFG.has_model()) {
            const auto& guideRateModel = guideCFG.model();

            const auto& targPhase = guideRateModel.target();
            const auto& allow_incr = guideRateModel.allow_increase();

            const auto it_nph = nph_enumToECL.find(targPhase);
            if (it_nph != nph_enumToECL.end()) {
                nom_phase = it_nph->second;
            }

            //nominated phase has negative sign for allow increment set to 'NO'
            if (!allow_incr) nom_phase *= -1;
        }

        return {nom_phase};
    }

    int getWhistctlMode(const ::Opm::Schedule& sched,
                        const std::size_t      report_step,
                        const std::size_t      lookup_step)
    {
        int mode = 0;
        if (report_step == std::size_t{0}) {
            return mode;
        }

        const auto& w_hist_ctl_mode = sched.getGlobalWhistctlMmode(lookup_step);
        const auto it_ctl = prod_cmodeToECL.find(w_hist_ctl_mode);
        if (it_ctl != prod_cmodeToECL.end()) {
            mode = it_ctl->second;
        }

        return mode;
    }

    int getLiftOptPar(const ::Opm::Schedule& sched,
                      const std::size_t      report_step,
                      const std::size_t      lookup_step)
    {
        using Value = ::Opm::RestartIO::Helpers::VectorItems::InteheadValues::LiftOpt;

        if (report_step == std::size_t{0}) {
            return Value::NotActive;
        }

        const auto& gasLiftOpt = sched.glo(lookup_step);
        if (! gasLiftOpt.active()) {
            return Value::NotActive;
        }

        return gasLiftOpt.all_newton()
            ? Value::EachNupCol
            : Value::FirstIterationOnly;
    }

    Opm::RestartIO::InteHEAD::ActiveNetwork
    getActiveNetwork(const Opm::Schedule&   sched,
                  const std::size_t      lookup_step)
    {
        const auto&  netwrk = sched[lookup_step].network();
        const auto actntwrk = netwrk.active() ? 2 : 0;
        return {
            actntwrk
        };
    }

    Opm::RestartIO::InteHEAD::NetworkDims
    getNetworkDims(const Opm::Schedule&   sched,
                  const std::size_t      lookup_step,
                  const ::Opm::Runspec& rspec)
    {
        const int noactnod = sched[lookup_step].network().node_names().size();
        const int noactbr  = sched[lookup_step].network().NoOfBranches();
        const int nodmax = std::max(rspec.networkDimensions().maxNONodes(), sched[lookup_step].network().NoOfNodes());
        const int nbrmax = std::max(rspec.networkDimensions().maxNoBranches(), sched[lookup_step].network().NoOfBranches());

        //the following dimensions are fixed
        const int nibran = 14;
        const int nrbran = 11;
        const int ninode = 10;
        const int nrnode = 17;
        const int nznode = 2;
        const int ninobr = 2*nbrmax;

        return {
            noactnod,
            noactbr,
            nodmax,
            nbrmax,
            nibran,
            nrbran,
            ninode,
            nrnode,
            nznode,
            ninobr
        };
    }

    Opm::RestartIO::InteHEAD::NetBalanceDims
    getNetworkBalanceParameters(const Opm::Schedule&   sched,
                  const std::size_t      report_step)
    {
        int maxNoItNBC = 0;
        int maxNoItTHP = 10;
        if (report_step > 0) {
            const auto& sched_state = sched[report_step];
            if (sched_state.network().active()) {
                const auto lookup_step = report_step - 1;
                const auto& netbal = sched[lookup_step].network_balance();
                maxNoItNBC = netbal.pressure_max_iter();
                maxNoItTHP  = netbal.thp_max_iter();
            }
        }
        return {
            maxNoItNBC,
            maxNoItTHP
        };
    }

    int maxGroupSize(const Opm::Schedule& sched,
                     const int            report_step,
                     const std::size_t    lookup_step,
                     const std::string_view lgr_tag)
    {
        if (lgr_tag.empty() || (lgr_tag == "GLOBAL")) {
            return (report_step == 0) ? 0 : Opm::maxGroupSize(sched, lookup_step);
        }

        return 1;
    }

} // Anonymous

// #####################################################################
// Public Interface (createInteHead()) Below Separator
// ---------------------------------------------------------------------

std::vector<int>
Opm::RestartIO::Helpers::
createInteHead(const EclipseState& es,
               const EclipseGrid&  grid,
               const Schedule&     sched,
               const double        simTime,
               const int           num_solver_steps,
               const int           report_step,
               const int           lookup_step)
{
    const auto nwgmax = ::maxGroupSize(sched, report_step,
                                       lookup_step,
                                       grid.get_lgr_tag());

    const auto ngmax  = (report_step == 0)
        ? 0 : numGroupsInField(sched, lookup_step, grid.get_lgr_tag());

    const auto& rspec = es.runspec();
    const auto& tdim  = es.getTableManager();
    const auto& rdim  = tdim.getRegdims();

    const auto tz = TracerSizes{rspec, es.getSimulationConfig()};

    const auto wellArrayDims = getWellArrayDims(tz);
    const auto connArrayDims = getConnArrayDims(tz);

    const auto ih = InteHEAD{}
        .dimensions         (grid.getNXYZ())
        .numActive          (static_cast<int>(grid.getNumActive()))
        .unitConventions    (es.getDeckUnitSystem())
        .wellTableDimensions(getWellTableDims(nwgmax, ngmax, rspec, sched,
                                              report_step, lookup_step,
                                              grid.get_lgr_tag()))
        .calendarDate       (getSimulationTimePoint(sched.posixStartTime(), simTime))
        .activePhases       (getActivePhases(rspec))
        .drsdt              (sched, lookup_step)
             // -----------------------------------------------------------------------------------
             //              NIWELZ               | NSWELZ               | NXWELZ               | NZWELZ
             //              #IWEL elems per well | #SWEL elems per well | #XWEL elems per well | #ZWEL elems per well
        .params_NWELZ       (wellArrayDims.iwell  , wellArrayDims.swell  , wellArrayDims.xwell  , wellArrayDims.zwell)
             // -----------------------------------------------------------------------------------
             //              NICONZ               | NSCONZ               | NXCONZ
             //              #ICON elems per conn | #SCON elems per conn | #XCON elems per conn
        .params_NCON        (connArrayDims.iconn  , connArrayDims.sconn  , connArrayDims.xconn)
        .params_GRPZ        (getNGRPZ(rspec, tz, nwgmax, ngmax))
        .aquiferDimensions  (inferAquiferDimensions(es, sched[lookup_step]))
        .stepParam          (num_solver_steps, report_step)
        .tuningParam        (getTuningPars(sched[lookup_step].tuning()))
        .liftOptParam       (getLiftOptPar(sched, report_step, lookup_step))
        .wellSegDimensions  (getWellSegDims(rspec, sched, tz, report_step, lookup_step))
        .regionDimensions   (getRegDims(tdim, rdim))
        .ngroups            ({ ngmax })
        .params_NGCTRL      (GroupControl(sched, report_step, lookup_step))
        .variousParam       (202204, 100)
        .udqParam_1         (getUdqParam(rspec, sched, report_step, lookup_step))
        .actionParam        (getActionParam(rspec, sched[lookup_step].actions(), report_step))
        .nominatedPhaseGuideRate(setGuideRateNominatedPhase(sched, report_step, lookup_step))
        .whistControlMode   (getWhistctlMode(sched, report_step, lookup_step))
        .activeNetwork      (getActiveNetwork(sched, lookup_step))
        .networkDimensions  (getNetworkDims(sched, lookup_step, rspec))
        .netBalanceData     (getNetworkBalanceParameters(sched, report_step))
        .rockOpts           (getRockOpts(es.getSimulationConfig().rock_config(), rdim))
        .tracerCounts       (rspec.tracers())
        ;

    return ih.data();
}
