/*
  Copyright 2019 Equinor ASA

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

#define BOOST_TEST_MODULE Array_Dimension_Checker

#include <boost/test/unit_test.hpp>

#include <boost/version.hpp>

#include <opm/input/eclipse/Schedule/ArrayDimChecker.hpp>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <iostream>
#include <memory>
#include <sstream>
#include <streambuf>
#include <string>

namespace {
    Opm::Deck simCaseWellDims()
    {
        const auto input = std::string{ R"(RUNSPEC
TITLE
  'Check Well Dimensions' /

DIMENS
  20 20 15 /

OIL
WATER

METRIC

EQLDIMS
-- Defaulted
/

TABDIMS
-- Defaulted
/

WELLDIMS
-- Defaulted
/

-- ====================================================================
GRID

SPECGRID
  20 20 15 1 F /

DXV
  20*100.0 /

DYV
  20*100.0 /

DZV
  15*0.1 /

DEPTHZ
  441*2000 /

PORO
  6000*0.3 /

PERMX
  6000*100.0 /

COPY
  'PERMX' 'PERMY' /
  'PERMX' 'PERMZ' /
/

MULTIPLY
  'PERMZ' 0.1 /
/

-- ====================================================================
PROPS

SWOF
  0 0 1 0
  1 1 0 0 /

PVDO
    1 1.0  0.5
  800 0.99 0.51 /

PVTW
  300 0.99 1.0e-6 0.25 0 /

DENSITY
  850.0 1014.0 1.05 /

-- ====================================================================
SOLUTION

EQUIL
  2000 300 2010 0.0 2000 10 /

-- ====================================================================
SUMMARY
ALL

-- ====================================================================
SCHEDULE

RPTRST
  BASIC=5 FREQ=6 /

GRUPTREE
  'G1'      'FIELD' /
  'PLAT1'   'G1'    /
  'PLAT2'   'G1'    /
  'I-NORTH' 'PLAT1' /
  'P-NORTH' 'PLAT1' /
  'O-WEST'  'PLAT2' /
  'I-SOUTH' 'PLAT2' /
  'P-EAST'  'PLAT2' /
  'G2'      'FIELD' /
  'PLAT3'   'G2'    /
  'I-2'     'PLAT3' /
/

WELSPECS
  'I-N-1' 'I-NORTH'  1  1  2000.15 'WATER' /
  'I-N-2' 'I-NORTH'  5  1  2001.05 'WATER' /
  'P-N-0' 'P-NORTH'  1 10  2000.15 'OIL'   /
  'P-N-1' 'P-NORTH' 10 15  2000.15 'OIL'   /
  'P-N-2' 'P-NORTH'  1 20  2000.15 'OIL'   /
  'P-N-3' 'P-NORTH' 19 20  2000.15 'OIL'   /
  'P-N-4' 'P-NORTH' 15 10  2000.15 'OIL'   /
  'P-N-5' 'P-NORTH' 10 10  2000.15 'OIL'   /
  'P-N-6' 'P-NORTH' 10 20  2000.15 'OIL'   /
  'P-N-7' 'P-NORTH'  7 15  2000.15 'OIL'   /
  'P-N-8' 'P-NORTH'  2 20  2000.15 'OIL'   /
  'P-N-9' 'P-NORTH' 20  1  2000.05 'OIL'   /
/

COMPDAT
  'I-N-1' 0 0  2 10 'OPEN' 1* 1* 1.0 /
  'I-N-2' 0 0 10 15 'OPEN' 1* 1* 1.0 /
  'P-N-0' 0 0  2  3 'OPEN' 1* 1* 1.0 /
  'P-N-1' 0 0  2  4 'OPEN' 1* 1* 1.0 /
  'P-N-2' 0 0  2  5 'OPEN' 1* 1* 1.0 /
  'P-N-3' 0 0  2  6 'OPEN' 1* 1* 1.0 /
  'P-N-4' 0 0  2  7 'OPEN' 1* 1* 1.0 /
  'P-N-5' 0 0  2  8 'OPEN' 1* 1* 1.0 /
  'P-N-6' 0 0  2  9 'OPEN' 1* 1* 1.0 /
  'P-N-7' 0 0  2 10 'OPEN' 1* 1* 1.0 /
  'P-N-8' 0 0  2  5 'OPEN' 1* 1* 1.0 /
  'P-N-9' 0 0  1 15 'OPEN' 1* 1* 1.0 /
/

WCONPROD
-- Well    O/S    Mode  ORAT  WRAT  GRAT  LRAT  RESV  BHP
  'P-N-*' 'OPEN' 'LRAT' 1*    1*    1*    5E3   1*    100 /
/

WCONINJE
-- Well    Type     O/S     Mode   RATE  RESV  BHP
  'I-N-*' 'WATER'  'OPEN'  'RATE'  25E3  1*    500 /
/

TSTEP
100*30 /

END
)" };

        return Opm::Parser{}.parseString(input);
    }

    Opm::Deck simCaseWellSegmentDims()
    {
        const auto input = std::string{ R"(RUNSPEC
TITLE
  Check Well Segment Dimensions /

DIMENS
  20 20 15 /

OIL
WATER

METRIC

EQLDIMS
-- Defaulted
/

TABDIMS
-- Defaulted
/

WELLDIMS
  2 10 5 5
/

WSEGDIMS
-- Defaulted => Max # MS wells = 0 (NSWLMX)
--              Max # segments = 1 (NSEGMX)
--              Max # branches = 1 (NLBRMX)
/

-- ====================================================================
GRID

SPECGRID
  20 20 15 1 F /

DXV
  20*100.0 /

DYV
  20*100.0 /

DZV
  15*0.1 /

DEPTHZ
  441*2000 /

PORO
  6000*0.3 /

PERMX
  6000*100.0 /

COPY
  'PERMX' 'PERMY' /
  'PERMX' 'PERMZ' /
/

MULTIPLY
  'PERMZ' 0.1 /
/

-- ====================================================================
PROPS

SWOF
  0 0 1 0
  1 1 0 0 /

PVDO
    1 1.0  0.5
  800 0.99 0.51 /

PVTW
  300 0.99 1.0e-6 0.25 0 /

DENSITY
  850.0 1014.0 1.05 /

-- ====================================================================
SOLUTION

EQUIL
  2000 300 2010 0.0 2000 10 /

-- ====================================================================
SUMMARY
ALL

-- ====================================================================
SCHEDULE

RPTRST
  BASIC=5 FREQ=6 /

GRUPTREE
  'G'       'FIELD' /
/

WELSPECS
  'I-N-1' 'G'  1  1  2000.15 'WATER' /
  'P-N-0' 'G'  1 10  2000.15 'OIL'   /
/

COMPDAT
  'I-N-1' 0 0  2 10 'OPEN' 1* 1* 1.0 /
  'P-N-0' 0 0  2 10 'OPEN' 1* 1* 1.0 /
/

WELSEGS
-- Number of MS wells =   1   (> 0 from WSEGDIMS(1))
-- Max segment ID     = 234   (> 1 from WSEGDIMS(2))
-- Max branch ID      = 123   (> 1 from WSEGDIMS(3))
 'P-N-0' 2345.6 2456.7 1* ABS 'HF-' /
 234 234 123 1 3456.7 2345.6 0.02468 0.0010000 /
/

COMPSEGS
   'P-N-0' /
 1 10  2 123 3456.7  3456.85 Z /
 1 10  3 123 3456.85 3457.0 Z /
 1 10  4 123 3457.0  3457.15 Z /
 1 10  5 123 3457.15 3457.3 Z /
 1 10  6 123 3457.3  3457.45 Z /
 1 10  7 123 3457.45 3457.6 Z /
 1 10  8 123 3457.6  3457.75 Z /
 1 10  9 123 3456.75 3457.90 Z /
 1 10 10 123 3456.90 3458.05 Z /
 /

WCONPROD
-- Well    O/S    Mode  ORAT  WRAT  GRAT  LRAT  RESV  BHP
  'P-N-*' 'OPEN' 'LRAT' 1*    1*    1*    5E3   1*    100 /
/

WCONINJE
-- Well    Type     O/S     Mode   RATE  RESV  BHP
  'I-N-*' 'WATER'  'OPEN'  'RATE'  25E3  1*    500 /
/

TSTEP
100*30 /

END
)" };

        return Opm::Parser{}.parseString(input);
    }

    Opm::Deck simCaseNodeGroupSizeFailure()
    {
        const auto input = std::string{ R"(RUNSPEC
TITLE
  'Check Well Dimensions' /

DIMENS
  20 20 15 /

OIL
WATER

METRIC

EQLDIMS
-- Defaulted
/

TABDIMS
-- Defaulted
/

WELLDIMS
-- NWMAX   NCMAX   NGMAX    NWGMAX (too small)
   2       20      16       4
/

-- ====================================================================
GRID

SPECGRID
  20 20 15 1 F /

DXV
  20*100.0 /

DYV
  20*100.0 /

DZV
  15*0.1 /

DEPTHZ
  441*2000 /

PORO
  6000*0.3 /

PERMX
  6000*100.0 /

COPY
  'PERMX' 'PERMY' /
  'PERMX' 'PERMZ' /
/

MULTIPLY
  'PERMZ' 0.1 /
/

-- ====================================================================
PROPS

SWOF
  0 0 1 0
  1 1 0 0 /

PVDO
    1 1.0  0.5
  800 0.99 0.51 /

PVTW
  300 0.99 1.0e-6 0.25 0 /

DENSITY
  850.0 1014.0 1.05 /

-- ====================================================================
SOLUTION

EQUIL
  2000 300 2010 0.0 2000 10 /

-- ====================================================================
SUMMARY
ALL

-- ====================================================================
SCHEDULE

RPTRST
  BASIC=5 FREQ=6 /

GRUPTREE
  'G1'      'FIELD' /
  'PLAT1'   'G1'    /
  'PLAT2'   'G1'    /
  'I-NORTH' 'PLAT1' /
  'P-NORTH' 'PLAT1' /
  'O-WEST'  'PLAT2' /
  'I-SOUTH' 'PLAT2' /
  'P-EAST'  'PLAT2' /
  'G2'      'FIELD' /
  'PLAT3'   'G2'    /
  'I-2'     'PLAT3' /
  'I-21'    'PLAT3' /
  'I-22'    'PLAT3' /
  'I-23'    'PLAT3' /
  'I-24'    'PLAT3' /
  'I-25'    'PLAT3' /
/

WELSPECS
  'I-N-1' 'I-NORTH'  1  1  2000.15 'WATER' /
  'P-N-0' 'P-NORTH'  1 10  2000.15 'OIL'   /
/

COMPDAT
  'I-N-1' 0 0  2 10 'OPEN' 1* 1* 1.0 /
  'P-N-0' 0 0  2  3 'OPEN' 1* 1* 1.0 /
/

WCONPROD
-- Well    O/S    Mode  ORAT  WRAT  GRAT  LRAT  RESV  BHP
  'P-N-*' 'OPEN' 'LRAT' 1*    1*    1*    5E3   1*    100 /
/

WCONINJE
-- Well    Type     O/S     Mode   RATE  RESV  BHP
  'I-N-*' 'WATER'  'OPEN'  'RATE'  25E3  1*    500 /
/

TSTEP
100*30 /

END
)" };

        return Opm::Parser{}.parseString(input);
    }
}

struct CaseObjects
{
    explicit CaseObjects(const Opm::Deck& deck, const Opm::ParseContext& ctxt);
    ~CaseObjects();

    CaseObjects(const CaseObjects& rhs) = default;
    CaseObjects(CaseObjects&& rhs) = default;

    CaseObjects& operator=(const CaseObjects& rhs) = delete;
    CaseObjects& operator=(CaseObjects&& rhs) = delete;

    Opm::ErrorGuard   guard;
    Opm::EclipseState es;
    Opm::Schedule     sched;
};

CaseObjects::CaseObjects(const Opm::Deck& deck, const Opm::ParseContext& ctxt)
    : guard {}
    , es    { deck }
    , sched { deck, es, ctxt, guard, std::make_shared<Opm::Python>() }
{}

CaseObjects::~CaseObjects()
{
    this->guard.clear();
}

class RedirectCERR
{
public:
    explicit RedirectCERR(std::streambuf* buf);
    ~RedirectCERR();

private:
    std::streambuf* orig_;
};

RedirectCERR::RedirectCERR(std::streambuf* buf)
    : orig_{ std::cerr.rdbuf(buf) }
{}

RedirectCERR::~RedirectCERR()
{
    std::cerr.rdbuf(this->orig_);
}

// ====================================================================

BOOST_AUTO_TEST_SUITE(WellDimensions)

namespace {
    void setWellDimsContext(const Opm::InputErrorAction action,
                            Opm::ParseContext&          ctxt)
    {
        ctxt.update(Opm::ParseContext::RUNSPEC_NUMWELLS_TOO_LARGE,       action);
        ctxt.update(Opm::ParseContext::RUNSPEC_CONNS_PER_WELL_TOO_LARGE, action);
        ctxt.update(Opm::ParseContext::RUNSPEC_NUMGROUPS_TOO_LARGE,      action);
        ctxt.update(Opm::ParseContext::RUNSPEC_GROUPSIZE_TOO_LARGE,      action);
    }
}

BOOST_AUTO_TEST_CASE(MaxGroupSize)
{
    Opm::ParseContext parseContext;
    auto cse = CaseObjects { simCaseWellDims(), parseContext };

    // Verify at most ten wells in a single group.
    BOOST_CHECK_EQUAL(Opm::maxGroupSize(cse.sched, 1), 10);
}

BOOST_AUTO_TEST_CASE(ManyChildGroups)
{
    auto cse = CaseObjects {
        simCaseNodeGroupSizeFailure(),
        Opm::ParseContext{}
    };

    // 6 Child groups of 'PLAT3'
    BOOST_CHECK_EQUAL(Opm::maxGroupSize(cse.sched, 1), 6);
}

BOOST_AUTO_TEST_CASE(WellDims)
{
    Opm::ParseContext parseContext;
    setWellDimsContext(Opm::InputErrorAction::THROW_EXCEPTION, parseContext);

    auto cse = CaseObjects { simCaseWellDims(), parseContext };

    // There should be no failures in basic input layer
    BOOST_CHECK(!cse.guard);

    BOOST_CHECK_THROW(Opm::checkConsistentArrayDimensions(cse.es      , cse.sched,
                                                          parseContext, cse.guard),
                      Opm::OpmInputError);

    setWellDimsContext(Opm::InputErrorAction::DELAYED_EXIT1, parseContext);
    Opm::checkConsistentArrayDimensions(cse.es      , cse.sched,
                                        parseContext, cse.guard);

    // There *should* be errors from dimension checking
    BOOST_CHECK_MESSAGE(cse.guard, "Exceeding WELLDIMS limits must produce errors");

    {
        std::stringstream estream;
        RedirectCERR stream(estream.rdbuf());

        cse.guard.dump();
        const auto error_msg = estream.str();

        for (const auto& s : {"RUNSPEC_NUMWELLS_TOO_LARGE"      , "item 1",
                              "RUNSPEC_CONNS_PER_WELL_TOO_LARGE", "item 2",
                              "RUNSPEC_NUMGROUPS_TOO_LARGE"     , "item 3",
                              "RUNSPEC_GROUPSIZE_TOO_LARGE"     , "item 4"})
        {
            BOOST_CHECK(error_msg.find(s) != std::string::npos);
        }

        BOOST_TEST_MESSAGE("WELLDIMS Diagnostic Message: '" << error_msg << '\'');
    }
}

BOOST_AUTO_TEST_CASE(WellDims_ManyChildGroups)
{
    Opm::ParseContext parseContext;
    setWellDimsContext(Opm::InputErrorAction::THROW_EXCEPTION, parseContext);

    auto cse = CaseObjects { simCaseNodeGroupSizeFailure(), parseContext };

    // There should be no failures in basic input layer
    BOOST_CHECK(!cse.guard);

    // There *should* be errors from dimension checking
    BOOST_CHECK_THROW(Opm::checkConsistentArrayDimensions(cse.es      , cse.sched,
                                                          parseContext, cse.guard),
                      Opm::OpmInputError);

    setWellDimsContext(Opm::InputErrorAction::DELAYED_EXIT1, parseContext);
    Opm::checkConsistentArrayDimensions(cse.es      , cse.sched,
                                        parseContext, cse.guard);

    // There *should* be errors from dimension checking
    BOOST_CHECK_MESSAGE(cse.guard, "Exceeding WELLDIMS limits must produce errors");

    // Verify that we get expected output from ErrorGuard::dump()
    {
        std::stringstream estream;
        RedirectCERR stream(estream.rdbuf());

        cse.guard.dump();
        const auto error_msg = estream.str();

        for (const auto& s : {"RUNSPEC_GROUPSIZE_TOO_LARGE", "item 4"}) {
            BOOST_CHECK(error_msg.find(s) != std::string::npos);
        }

        BOOST_TEST_MESSAGE("WELLDIMS Diagnostic Message: '" << error_msg << '\'');
    }
}

BOOST_AUTO_TEST_SUITE_END()

// ====================================================================

BOOST_AUTO_TEST_SUITE(WellSegmentDimensions)

namespace {
    void setWellSegmentDimsContext(const Opm::InputErrorAction action,
                                   Opm::ParseContext&          ctxt)
    {
        ctxt.update(Opm::ParseContext::RUNSPEC_NUMMSW_TOO_LARGE,          action);
        ctxt.update(Opm::ParseContext::RUNSPEC_NUMSEG_PER_WELL_TOO_LARGE, action);
        ctxt.update(Opm::ParseContext::RUNSPEC_NUMBRANCH_TOO_LARGE,       action);
    }
}

BOOST_AUTO_TEST_CASE(WellSegDims)
{
    auto parseContext = Opm::ParseContext{};
    setWellSegmentDimsContext(Opm::InputErrorAction::THROW_EXCEPTION, parseContext);

    auto cse = CaseObjects { simCaseWellSegmentDims(), parseContext };

    // There should be no failures in basic input layer
    BOOST_CHECK_MESSAGE(! cse.guard, "Reading input file must not produce errors");

    // Default action (THROW_EXCEPTION) must throw exception from array
    // dimension checking.
    BOOST_CHECK_THROW(Opm::checkConsistentArrayDimensions(cse.es      , cse.sched,
                                                          parseContext, cse.guard),
                      Opm::OpmInputError);

    setWellSegmentDimsContext(Opm::InputErrorAction::DELAYED_EXIT1, parseContext);
    Opm::checkConsistentArrayDimensions(cse.es      , cse.sched,
                                        parseContext, cse.guard);

    // There *should* be errors from dimension checking when action is DELAYED_EXIT1.
    BOOST_CHECK_MESSAGE(cse.guard, "Exceeding WSEGDIMS limits must produce errors");

    {
        std::stringstream estream;
        RedirectCERR stream(estream.rdbuf());

        using namespace std::string_literals;

        cse.guard.dump();
        const auto error_msg = estream.str();

        for (const auto* s : {"RUNSPEC_NUMMSW_TOO_LARGE"         , "item 1",
                              "RUNSPEC_NUMSEG_PER_WELL_TOO_LARGE", "item 2",
                              "RUNSPEC_NUMBRANCH_TOO_LARGE"      , "item 3"})
        {
            BOOST_CHECK_MESSAGE(error_msg.find(s) != std::string::npos,
                                "Diagnostic message element '"s + s +
                                "' must be in WSEGDIMS error message"s);
        }

        BOOST_TEST_MESSAGE("WSEGDIMS Diagnostic Message: '" << error_msg << '\'');
    }
}

BOOST_AUTO_TEST_SUITE_END() // WellSegmentDimensions
