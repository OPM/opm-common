/*
  Copyright 2018 Statoil IT

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

#define BOOST_TEST_MODULE InteHEAD_Vector

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/InteHEAD.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <iterator>
#include <numeric>              // partial_sum()
#include <string>
#include <vector>

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;

namespace {
    std::vector<double> elapsedTime(const Opm::TimeMap& tmap)
    {
        auto elapsed = std::vector<double>{};

        elapsed.reserve(tmap.numTimesteps() + 1);
        elapsed.push_back(0.0);

        for (auto nstep = tmap.numTimesteps(),
                  step  = 0*nstep; step < nstep; ++step)
        {
            elapsed.push_back(tmap.getTimeStepLength(step));
        }

        std::partial_sum(std::begin(elapsed), std::end(elapsed),
                         std::begin(elapsed));

        return elapsed;
    }

    void expectDate(const Opm::RestartIO::InteHEAD::TimePoint& tp,
                    const int year, const int month, const int day)
    {
        BOOST_CHECK_EQUAL(tp.year  , year);
        BOOST_CHECK_EQUAL(tp.month , month);
        BOOST_CHECK_EQUAL(tp.day   , day);

        BOOST_CHECK_EQUAL(tp.hour        , 0);
        BOOST_CHECK_EQUAL(tp.minute      , 0);
        BOOST_CHECK_EQUAL(tp.second      , 0);
        BOOST_CHECK_EQUAL(tp.microseconds, 0);
    }
} // Anonymous

BOOST_AUTO_TEST_SUITE(Member_Functions)

BOOST_AUTO_TEST_CASE(Dimensions_Individual)
{
    const auto ih = Opm::RestartIO::InteHEAD{}
        .dimensions(100, 60, 15);

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NX], 100);
    BOOST_CHECK_EQUAL(v[VI::intehead::NY],  60);
    BOOST_CHECK_EQUAL(v[VI::intehead::NZ],  15);
}

BOOST_AUTO_TEST_CASE(Dimensions_Array)
{
    const auto ih = Opm::RestartIO::InteHEAD{}
        .dimensions({ {100, 60, 15} });

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NX], 100);
    BOOST_CHECK_EQUAL(v[VI::intehead::NY],  60);
    BOOST_CHECK_EQUAL(v[VI::intehead::NZ],  15);
}

BOOST_AUTO_TEST_CASE(NumActive)
{
    const auto ih = Opm::RestartIO::InteHEAD{}
        .numActive(72390);

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NACTIV], 72390);
}

BOOST_AUTO_TEST_CASE(UnitConventions)
{
    using USys = Opm::RestartIO::InteHEAD::UnitSystem;

    auto ih = Opm::RestartIO::InteHEAD{};

    // Metric
    {
        ih.unitConventions(USys::Metric);

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::UNIT], 1);
    }

    // Field
    {
        ih.unitConventions(USys::Field);

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::UNIT], 2);
    }

    // Lab
    {
        ih.unitConventions(USys::Lab);

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::UNIT], 3);
    }

    // PVT-M
    {
        ih.unitConventions(USys::PVT_M);

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::UNIT], 4);
    }
}

BOOST_AUTO_TEST_CASE(WellTableDimensions)
{
    const auto numWells        = 17;
    const auto maxPerf         = 29;
    const auto maxWellInGroup  =  3;
    const auto maxGroupInField = 14;

    const auto ih = Opm::RestartIO::InteHEAD{}
        .wellTableDimensions({
            numWells, maxPerf, maxWellInGroup, maxGroupInField
        });

    const auto& v = ih.data();
    const auto nwgmax = std::max(maxWellInGroup, maxGroupInField);

    BOOST_CHECK_EQUAL(v[VI::intehead::NWELLS], numWells);
    BOOST_CHECK_EQUAL(v[VI::intehead::NCWMAX], maxPerf);
    BOOST_CHECK_EQUAL(v[VI::intehead::NWGMAX], nwgmax);
    BOOST_CHECK_EQUAL(v[VI::intehead::NGMAXZ], maxGroupInField + 1);
}

BOOST_AUTO_TEST_CASE(CalendarDate)
{
    // 2015-04-09T11:22:33.987654+0000

    const auto ih = Opm::RestartIO::InteHEAD{}
        .calendarDate({
            2015, 4, 9, 11, 22, 33, 987654,
        });

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[67 - 1], 2015); // Year
    BOOST_CHECK_EQUAL(v[66 - 1],    4); // Month
    BOOST_CHECK_EQUAL(v[65 - 1],    9); // Day

    BOOST_CHECK_EQUAL(v[207 - 1], 11); // Hour
    BOOST_CHECK_EQUAL(v[208 - 1], 22); // Minute
    BOOST_CHECK_EQUAL(v[411 - 1], 33987654); // Second (in microseconds)
}

BOOST_AUTO_TEST_CASE(ActivePhases)
{
    using Ph = Opm::RestartIO::InteHEAD::Phases;
    auto  ih = Opm::RestartIO::InteHEAD{};

    // Oil
    {
        ih.activePhases(Ph{ 1, 0, 0 });

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::PHASE], 1);
    }

    // Water
    {
        ih.activePhases(Ph{ 0, 1, 0 });

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::PHASE], 2);
    }

    // Gas
    {
        ih.activePhases(Ph{ 0, 0, 1 });

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::PHASE], 4);
    }

    // Oil/Water
    {
        ih.activePhases(Ph{ 1, 1, 0 });

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::PHASE], 3);
    }

    // Oil/Gas
    {
        ih.activePhases(Ph{ 1, 0, 1 });

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::PHASE], 5);
    }

    // Water/Gas
    {
        ih.activePhases(Ph{ 0, 1, 1 });

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::PHASE], 6);
    }

    // Oil/Water/Gas
    {
        ih.activePhases(Ph{ 1, 1, 1 });

        const auto& v = ih.data();

        BOOST_CHECK_EQUAL(v[VI::intehead::PHASE], 7);
    }
}

BOOST_AUTO_TEST_CASE(NWell_Parameters)
{
    const auto ih = Opm::RestartIO::InteHEAD{}
      .params_NWELZ(27, 18, 28, 1);

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NIWELZ], 27);
    BOOST_CHECK_EQUAL(v[VI::intehead::NSWELZ], 18);
    BOOST_CHECK_EQUAL(v[VI::intehead::NXWELZ], 28);
    BOOST_CHECK_EQUAL(v[VI::intehead::NZWELZ],  1);
}

BOOST_AUTO_TEST_CASE(NConn_Parameters)
{
    const auto ih = Opm::RestartIO::InteHEAD{}
        .params_NCON(31, 41, 59);

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NICONZ], 31);
    BOOST_CHECK_EQUAL(v[VI::intehead::NSCONZ], 41);
    BOOST_CHECK_EQUAL(v[VI::intehead::NXCONZ], 59);
}

BOOST_AUTO_TEST_CASE(GroupSize_Parameters)
{
    // https://oeis.org/A001620
    const auto ih = Opm::RestartIO::InteHEAD{}
        .params_GRPZ({
            { 577, 215, 664, 901 }
        });

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NIGRPZ], 577);
    BOOST_CHECK_EQUAL(v[VI::intehead::NSGRPZ], 215);
    BOOST_CHECK_EQUAL(v[VI::intehead::NXGRPZ], 664);
    BOOST_CHECK_EQUAL(v[VI::intehead::NZGRPZ], 901);
}

BOOST_AUTO_TEST_CASE(Analytic_Aquifer_Parameters)
{
    // https://oeis.org/A001622
    const auto ih = Opm::RestartIO::InteHEAD{}
        .params_NAAQZ(1, 61, 803, 3988, 74989, 484820, 4586834);

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NCAMAX], 1);
    BOOST_CHECK_EQUAL(v[VI::intehead::NIAAQZ], 61);
    BOOST_CHECK_EQUAL(v[VI::intehead::NSAAQZ], 803);
    BOOST_CHECK_EQUAL(v[VI::intehead::NXAAQZ], 3988);
    BOOST_CHECK_EQUAL(v[VI::intehead::NICAQZ], 74989);
    BOOST_CHECK_EQUAL(v[VI::intehead::NSCAQZ], 484820);
    BOOST_CHECK_EQUAL(v[VI::intehead::NACAQZ], 4586834);
}

BOOST_AUTO_TEST_CASE(Time_and_report_step)
{
    const auto ih = Opm::RestartIO::InteHEAD{}
        .stepParam(12, 2);

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[67], 12); // TSTEP
    BOOST_CHECK_EQUAL(v[68],  2); // REP_STEP
}

BOOST_AUTO_TEST_CASE(Tuning_param)
{
    const auto newtmx	= 17;
    const auto newtmn	=  5;
    const auto litmax	= 102;
    const auto litmin	= 20;
    const auto mxwsit	= 8;
    const auto mxwpit	= 6;

    const auto ih = Opm::RestartIO::InteHEAD{}
        .tuningParam({
            newtmx, newtmn, litmax, litmin, mxwsit, mxwpit
        });

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[80], newtmx);        // NEWTMX
    BOOST_CHECK_EQUAL(v[81], newtmn);        // NEWTMN
    BOOST_CHECK_EQUAL(v[82], litmax);        // LITMAX
    BOOST_CHECK_EQUAL(v[83], litmin);        // LITMIN
    BOOST_CHECK_EQUAL(v[86], mxwsit);        // MXWSIT
    BOOST_CHECK_EQUAL(v[87], mxwpit);        // MXWPIT
}

BOOST_AUTO_TEST_CASE(Various_Parameters)
{
    const auto ih = Opm::RestartIO::InteHEAD{}
        .variousParam(2015, 100);

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[  1], 2015); // VERSION
    BOOST_CHECK_EQUAL(v[ 94], 100); // IPROG
    BOOST_CHECK_EQUAL(v[ 76],   5); // IH_076
    BOOST_CHECK_EQUAL(v[101],   1); // IH_101
    BOOST_CHECK_EQUAL(v[103],   1); // IH_103
}

BOOST_AUTO_TEST_CASE(wellSegDimensions)
{
    const auto nsegwl = 3;
    const auto nswlmx = 4;
    const auto nsegmx = 5;
    const auto nlbrmx = 6;
    const auto nisegz = 7;
    const auto nrsegz = 8;
    const auto nilbrz = 9;

    const auto ih = Opm::RestartIO::InteHEAD{}
        .wellSegDimensions({
            nsegwl, nswlmx, nsegmx, nlbrmx, nisegz, nrsegz, nilbrz
        });

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[VI::intehead::NSEGWL], nsegwl);
    BOOST_CHECK_EQUAL(v[VI::intehead::NSWLMX], nswlmx);
    BOOST_CHECK_EQUAL(v[VI::intehead::NSEGMX], nsegmx);
    BOOST_CHECK_EQUAL(v[VI::intehead::NLBRMX], nlbrmx);
    BOOST_CHECK_EQUAL(v[VI::intehead::NISEGZ], nisegz);
    BOOST_CHECK_EQUAL(v[VI::intehead::NRSEGZ], nrsegz);
    BOOST_CHECK_EQUAL(v[VI::intehead::NILBRZ], nilbrz);
}

BOOST_AUTO_TEST_CASE(regionDimensions)
{
    const auto ntfip  = 12;
    const auto nmfipr = 22;
    const auto nrfreg = 5;
    const auto ntfreg = 6;
    const auto nplmix = 7;

    const auto ih = Opm::RestartIO::InteHEAD{}
        .regionDimensions({
            ntfip, nmfipr, nrfreg, ntfreg, nplmix
        });

    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[89], ntfip);  // NTFIP
    BOOST_CHECK_EQUAL(v[99], nmfipr); // NMFIPR
}

BOOST_AUTO_TEST_CASE(ngroups)
{
    const auto ngroup  = 8;

    const auto ih = Opm::RestartIO::InteHEAD{}
        .ngroups({ ngroup });
	
    const auto& v = ih.data();

    BOOST_CHECK_EQUAL(v[18], ngroup);  // NGRP
}
BOOST_AUTO_TEST_CASE(SimulationDate)
{
    const auto input = std::string { R"(
RUNSPEC

START
  1 JAN 2000
/

SCHEDULE

DATES
  1 'JAN' 2001 /
/

TSTEP
--Advance the simulater for TEN years:
  10*365.0D0 /
)"  };

    const auto tmap = ::Opm::TimeMap {
        ::Opm::Parser{}.parseString(input)
    };

    const auto start   = tmap.getStartTime(0);
    const auto elapsed = elapsedTime(tmap);

    auto checkDate = [start, &elapsed]
        (const std::vector<double>::size_type i,
         const std::array<int, 3>&            expectYMD) -> void
    {
        using ::Opm::RestartIO::getSimulationTimePoint;

        expectDate(getSimulationTimePoint(start, elapsed[i]),
                   expectYMD[0], expectYMD[1], expectYMD[2]);
    };

    // START
    checkDate(0, { 2000, 1, 1 });  // Start   == 2000-01-01

    // DATES (2000 being leap year is immaterial)
    checkDate(1, { 2001, 1, 1 });  // RStep 1 == 2000-01-01 -> 2001-01-01

    // TSTEP
    checkDate(2, { 2002, 1, 1 });  // RStep 2 == 2001-01-01 -> 2002-01-01
    checkDate(3, { 2003, 1, 1 });  // RStep 3 == 2002-01-01 -> 2003-01-01
    checkDate(4, { 2004, 1, 1 });  // RStep 4 == 2003-01-01 -> 2004-01-01

    // Leap year: 2004
    checkDate(5, { 2004, 12, 31 }); // RStep 5 == 2004-01-01 -> 2004-12-31
    checkDate(6, { 2005, 12, 31 }); // RStep 6 == 2004-12-31 -> 2005-12-31
    checkDate(7, { 2006, 12, 31 }); // RStep 7 == 2005-12-31 -> 2006-12-31
    checkDate(8, { 2007, 12, 31 }); // RStep 8 == 2006-12-31 -> 2007-12-31

    // Leap year: 2008
    checkDate( 9, { 2008, 12, 30 }); // RStep  9 == 2007-12-31 -> 2008-12-30
    checkDate(10, { 2009, 12, 30 }); // RStep 10 == 2008-12-30 -> 2009-12-30
    checkDate(11, { 2010, 12, 30 }); // RStep 11 == 2009-12-30 -> 2010-12-30
}

BOOST_AUTO_TEST_SUITE_END()
