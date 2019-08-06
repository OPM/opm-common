/*
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

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/RestartConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ArrayDimChecker.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Regdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {
    int maxConnPerWell(const Opm::Schedule& sched,
                       const std::size_t    lookup_step)
    {
        auto ncwmax = 0;

        for (const auto& well : sched.getWells2(lookup_step)) {
            const auto ncw = well.getConnections().size();

            ncwmax = std::max(ncwmax, static_cast<int>(ncw));
        }

        return ncwmax;
    }

    int numGroupsInField(const Opm::Schedule& sched,
                         const std::size_t    lookup_step)
    {
        const auto ngmax = sched.numGroups(lookup_step);

        if (ngmax < 1) {
            throw std::invalid_argument {
                "Simulation run must include at least FIELD group"
            };
        }

        // Number of non-FIELD groups.
        return ngmax - 1;
    }

    Opm::RestartIO::InteHEAD::WellTableDim
    getWellTableDims(const int              nwgmax,
                     const int              ngmax,
                     const ::Opm::Runspec&  rspec,
                     const ::Opm::Schedule& sched,
                     const std::size_t      lookup_step)
    {
        const auto& wd = rspec.wellDimensions();

        const auto numWells = static_cast<int>(sched.numWells(lookup_step));

        const auto maxPerf =
            std::max(wd.maxConnPerWell(),
                     maxConnPerWell(sched, lookup_step));

        const auto maxWellInGroup =
            std::max(wd.maxWellsPerGroup(), nwgmax);

        const auto maxGroupInField =
            std::max(wd.maxGroupsInField(), ngmax);

        return {
            numWells,
            maxPerf,
            maxWellInGroup,
            maxGroupInField,
        };
    }

    std::array<int, 4>
    getNGRPZ(const int             grpsz,
             const int             ngrp,
             const ::Opm::Runspec& rspec)
    {
        const auto& wd = rspec.wellDimensions();

        const auto nwgmax = std::max(grpsz, wd.maxWellsPerGroup());
        const auto ngmax  = std::max(ngrp , wd.maxGroupsInField());

        const int nigrpz = 97 + std::max(nwgmax, ngmax);
        const int nsgrpz = 112;
        const int nxgrpz = 180;
        const int nzgrpz = 5;

        return {{
            nigrpz,
            nsgrpz,
            nxgrpz,
            nzgrpz,
        }};
    }

    Opm::RestartIO::InteHEAD::UnitSystem
    getUnitConvention(const ::Opm::UnitSystem& us)
    {
        using US = ::Opm::RestartIO::InteHEAD::UnitSystem;

        switch (us.getType()) {
        case ::Opm::UnitSystem::UnitType::UNIT_TYPE_METRIC:
            return US::Metric;

        case ::Opm::UnitSystem::UnitType::UNIT_TYPE_FIELD:
            return US::Field;

        case ::Opm::UnitSystem::UnitType::UNIT_TYPE_LAB:
            return US::Lab;

        case ::Opm::UnitSystem::UnitType::UNIT_TYPE_PVT_M:
            return US::PVT_M;

        case ::Opm::UnitSystem::UnitType::UNIT_TYPE_INPUT:
            throw std::invalid_argument {
                "Cannot Run Simulation With Non-Standard Units"
            };
        }

        return US::Metric;
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
    getTuningPars(const ::Opm::Tuning& tuning,
                  const std::size_t    lookup_step)
    {
        const auto& newtmx = tuning.getNEWTMX(lookup_step);
        const auto& newtmn = tuning.getNEWTMN(lookup_step);
        const auto& litmax = tuning.getLITMAX(lookup_step);
        const auto& litmin = tuning.getLITMIN(lookup_step);
        const auto& mxwsit = tuning.getMXWSIT(lookup_step);
        const auto& mxwpit = tuning.getMXWPIT(lookup_step);

        return {
            newtmx,
            newtmn,
            litmax,
            litmin,
            mxwsit,
            mxwpit,
        };
    }

    Opm::RestartIO::InteHEAD::UdqParam
    getRandSeedPar(const ::Opm::Runspec& rspec)
    { 
        const auto& udq_par = rspec.udqParams();
        const auto r_seed = udq_par.rand_seed();
        
        return { r_seed};
    }
    
    Opm::RestartIO::InteHEAD::WellSegDims
    getWellSegDims(const ::Opm::Runspec&  rspec,
                   const ::Opm::Schedule& sched,
                   const std::size_t      lookup_step)
    {
	const auto& wsd = rspec.wellSegmentDimensions();

        const auto& sched_wells = sched.getWells2(lookup_step);

        const auto nsegwl =
            std::count_if(std::begin(sched_wells), std::end(sched_wells),
                          [](const Opm::Well2& well)
            {
                return well.isMultiSegment();
            });

        const auto nswlmx = wsd.maxSegmentedWells();
        const auto nsegmx = wsd.maxSegmentsPerWell();
        const auto nlbrmx = wsd.maxLateralBranchesPerWell();
        const auto nisegz = 22;  // Number of entries per segment in ISEG.
        const auto nrsegz = 146; // Number of entries per segment in RSEG array.  (Eclipse v.2017)
        const auto nilbrz = 10;  // Number of entries per segment in ILBR array.

        return {
            static_cast<int>(nsegwl),
            nswlmx,
            nsegmx,
            nlbrmx,
            nisegz,
            nrsegz,
            nilbrz
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
               const int           lookup_step)
{
    const auto  nwgmax = maxGroupSize(sched, lookup_step);
    const auto  ngmax  = numGroupsInField(sched, lookup_step);
    const auto& rspec  = es.runspec();
    const auto& tdim   = es.getTableManager();
    const auto& rdim   = tdim.getRegdims();

    const auto ih = InteHEAD{}
        .dimensions         (grid.getNXYZ())
        .numActive          (static_cast<int>(grid.getNumActive()))
        .unitConventions    (getUnitConvention(es.getDeckUnitSystem()))
        .wellTableDimensions(getWellTableDims(nwgmax, ngmax, rspec,
                                              sched, lookup_step))
        .calendarDate       (getSimulationTimePoint(sched.posixStartTime(), simTime))
        .activePhases       (getActivePhases(rspec))
             // The numbers below have been determined experimentally to work
             // across a range of reference cases, but are not guaranteed to be
             // universally valid.
        .params_NWELZ       (155, 122, 130, 3) // n{isxz}welz: number of data elements per well in {ISXZ}WELL
        .params_NCON        (25, 40, 58)       // n{isx}conz: number of data elements per completion in ICON
        .params_GRPZ        (getNGRPZ(nwgmax, ngmax, rspec))
             // ncamax: max number of analytical aquifer connections
             // n{isx}aaqz: number of data elements per aquifer in {ISX}AAQ
             // n{isa}caqz: number of data elements per aquifer connection in {ISA}CAQ
        .params_NAAQZ       (1, 18, 24, 10, 7, 2, 4)
        .stepParam          (num_solver_steps, lookup_step)
        .tuningParam        (getTuningPars(sched.getTuning(), lookup_step))
        .wellSegDimensions  (getWellSegDims(rspec, sched, lookup_step))
        .regionDimensions   (getRegDims(tdim, rdim))
        .ngroups            ({ ngmax })
        .variousParam       (201702, 100)  // Output should be compatible with Eclipse 100, 2017.02 version.
        .udqParam_1         (getRandSeedPar(rspec))
        ;

    return ih.data();
}
