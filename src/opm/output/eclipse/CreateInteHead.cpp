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
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Regdims.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <stdexcept>
#include <vector>

namespace {
    Opm::RestartIO::InteHEAD::WellTableDim
    getWellTableDims(const ::Opm::Runspec&  rspec,
                     const ::Opm::Schedule& sched)
    {
        const auto& td = rspec.tabdims();
        const auto& wd = rspec.wellDimensions();

        const auto numWells = static_cast<int>(sched.numWells());

        const auto maxPerf         = wd.maxConnPerWell();
        const auto maxWellInGroup  = wd.maxWellsPerGroup();
        const auto maxGroupInField = wd.maxGroupsInField();

        return {
            numWells,
            maxPerf,
            maxWellInGroup,
            maxGroupInField,
        };
    }

    std::array<int, 4> getNGRPZ(const ::Opm::Runspec& rspec)
    {
        const auto& wd = rspec.wellDimensions();
        const auto maxWellInGroup  = wd.maxWellsPerGroup();
        const int nigrpz = 97 + maxWellInGroup;
        const int nsgrpz = 112;
        const int nxgrpz = 180;
        const int nzgrpz = 5;

        return {
            nigrpz,
            nsgrpz,
            nxgrpz,
            nzgrpz
        };
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

    Opm::RestartIO::InteHEAD::Date
    getSimulationDate(const ::Opm::Schedule& sched,
                      const double           simTime)
    {
        // Would have liked to use H.Hinnant's "date" library here
        // (https://github.com/HowardHinnant/date), especially to avoid
        // dealing with std::time_t and the threading issues of calendar
        // function std::gmtime().  Library proposed (LEWG) for C++20.
        //
        // Pray that we're on a system for which std::time_t is 64 bits (or
        // more) so that we at least don't run into 2k38 with the typical
        // definition of time_point::time_since_epoch(system_clock::now())

        using namespace std::chrono;

        const auto start = system_clock::
            from_time_t(sched.posixStartTime());

        // Discard sub-second resolution.
        const auto now = start + duration_cast<seconds>(
            duration<double, seconds::period>(simTime));

        const auto t = system_clock::to_time_t(now);

        // Not thread-safe (returns pointer to internal, static storage).
        // May wish to add some locking here...
        const auto timepoint = *std::localtime(&t);

        return {
            timepoint.tm_year + 1900,
            timepoint.tm_mon  + 1,
            timepoint.tm_mday ,
            timepoint.tm_hour ,
            timepoint.tm_min  ,
            std::min(timepoint.tm_sec, 59), // Ignore leap seconds...
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
    getTuningPars(const ::Opm::Tuning&  tuning, const size_t step)
    {
        const auto& newtmx = tuning.getNEWTMX(step);
	const auto& newtmn = tuning.getNEWTMN(step);
	const auto& litmax = tuning.getLITMAX(step);
	const auto& litmin = tuning.getLITMIN(step);
	const auto& mxwsit = tuning.getMXWSIT(step);
	const auto& mxwpit = tuning.getMXWPIT(step);

        return {
            newtmx,
            newtmn,
	    litmax,
	    litmin,
            mxwsit,
            mxwpit,
        };
    }
    
    Opm::RestartIO::InteHEAD::WellSegDims
    getWellSegDims(const ::Opm::Runspec&  rspec,
		   const ::Opm::Schedule sched,
		   const size_t step)
    {
        const auto& wsd = rspec.wellSegmentDimensions();
	const auto& sched_wells = sched.getWells( step );
	int n_act_seg_wels = 0; 
	for (const auto& wl : sched_wells)  {
	  /*if (wl->isMultiSegment(step)) {
	    std::cout << "sched_well wl: " << wl->name() << std::endl;
	    n_act_seg_wels += 1;
	  }
	  else {
	    std::cout << "non_sched_well wl: " << wl->name() << std::endl;
	  }*/
	    n_act_seg_wels = (wl->isMultiSegment(step))
	      ? n_act_seg_wels +1 : n_act_seg_wels;
	}

	const auto nsegwl	= n_act_seg_wels;
        const auto nswlmx	= wsd.maxSegmentedWells();
        const auto nsegmx	= wsd.maxSegmentsPerWell();
        const auto nlbrmx 	= wsd.maxLateralBranchesPerWell();
	const auto nisegz 	= 22;
	const auto nrsegz	= 140;
	const auto nilbrz 	= 10;

        return {
            nsegwl,
            nswlmx,
            nsegmx,
            nlbrmx,
	    nisegz,
	    nrsegz,
	    nilbrz
        };
    }
    
    Opm::RestartIO::InteHEAD::RegDims
    getRegDims(const ::Opm::TableManager tdims,  const ::Opm::Regdims&  rdims)
    {
        const auto& ntfip  = tdims.numFIPRegions();
	const auto& nmfipr = rdims.getNMFIPR();
	const auto& nrfreg = rdims.getNRFREG();
	const auto& ntfreg = rdims.getNTFREG();
	const auto& nplmix = rdims.getNPLMIX();


        return {
            ntfip,
            nmfipr,
            nrfreg,
            ntfreg,
	    nplmix
        };
    }
} 
// Anonymous

// #####################################################################
// Public Interface (createInteHead()) Below Separator
// ---------------------------------------------------------------------

std::vector<int>
Opm::RestartIO::Helpers::
createInteHead(const EclipseState& es,
               const EclipseGrid&  grid,
               const Schedule&     sched,
               const double        simTime,
               const int           report_step)
{
    const auto& rspec = es.runspec();
    const auto& tdim = es.getTableManager();
    const auto& rdim = tdim.getRegdims();

    const auto ih = InteHEAD{}
        .dimensions         (grid.getNXYZ())
        .numActive          (static_cast<int>(grid.getNumActive()))
        .unitConventions    (getUnitConvention(es.getDeckUnitSystem()))
        .wellTableDimensions(getWellTableDims(rspec, sched))
        .calenderDate       (getSimulationDate(sched, simTime))
        .activePhases       (getActivePhases(rspec))
        .params_NWELZ       (155, 122, 130, 3)
        .params_NCON        (25, 40, 58)
        .params_GRPZ        (getNGRPZ(rspec))
        .params_NAAQZ       (1, 18, 24, 10, 7, 2, 4)
	.stepParam(report_step, report_step)
	.tuningParam(getTuningPars(sched.getTuning(), report_step))
	.wellSegDimensions(getWellSegDims(rspec, sched, report_step))
	.regionDimensions(getRegDims(tdim, rdim))
	.variousParam(100, 1, 1)
	;

    return ih.data();
}
