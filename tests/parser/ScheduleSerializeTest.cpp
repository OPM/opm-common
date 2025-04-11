/*
  Copyright 2021 Equinor ASA.

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

#define BOOST_TEST_MODULE ScheduleTests

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/MemPacker.hpp>
#include <opm/common/utility/Serializer.hpp>
#include <opm/common/utility/TimeService.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/Action/ASTNode.hpp>
#include <opm/input/eclipse/Schedule/GasLiftOpt.hpp>
#include <opm/input/eclipse/Schedule/Group/GConSale.hpp>
#include <opm/input/eclipse/Schedule/Group/GConSump.hpp>
#include <opm/input/eclipse/Schedule/Group/GSatProd.hpp>
#include <opm/input/eclipse/Schedule/Group/GroupEconProductionLimits.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRate.hpp>
#include <opm/input/eclipse/Schedule/Group/GuideRateConfig.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Network/Balance.hpp>
#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>
#include <opm/input/eclipse/Schedule/RFTConfig.hpp>
#include <opm/input/eclipse/Schedule/RPTConfig.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQActive.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQASTNode.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvg.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellBrineProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEconProductionLimits.hpp>
#include <opm/input/eclipse/Schedule/Well/WellFoamProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellFractureSeeds.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMICPProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTracerProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPDP.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPEXP.hpp>

#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

using namespace Opm;

std::string WTEST_deck = R"(
START             -- 0
10 MAI 2007 /
GRID
PORO
    1000*0.1 /
PERMX
    1000*1 /
PERMY
    1000*0.1 /
PERMZ
    1000*0.01 /
SCHEDULE
WELSPECS
     'DEFAULT'    'OP'   30   37  3.33       'OIL'  7*/
     'ALLOW'      'OP'   30   37  3.33       'OIL'  3*  YES /
     'BAN'        'OP'   20   51  3.92       'OIL'  3*  NO /
     'W1'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'W2'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'W3'         'OP'   20   51  3.92       'OIL'  3*  NO /
/

COMPDAT
 'BAN'  1  1   1   1 'OPEN' 1*    1.168   0.311   107.872 1*  1*  'Z'  21.925 /
/

WCONHIST
     'BAN'      'OPEN'      'RESV'      0.000      0.000      0.000  5* /
/

WTEST
   'ALLOW'   1   'PE' /
/

WLIST
  '*ILIST'  'NEW'  W1 /
  '*ILIST'  'ADD'  W2 /
/

DATES             -- 1
 10  JUN 2007 /
/

DATES             -- 2
 15  JUN 2007 /
/

DATES             -- 3
 20  JUN 2007 /
/


WTEST
   'ALLOW'  1  '' /
   'BAN'    1  'DGC' /
/

WLIST
  '*ILIST'  'ADD'  W3 /
/


WCONHIST
     'BAN'      'OPEN'      'RESV'      1.000      0.000      0.000  5* /
/

DATES             -- 4
 10  JUL 2007 /
/

WELSPECS
     'I1'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'I2'         'OP'   20   51  3.92       'OIL'  3*  NO /
     'I3'         'OP'   20   51  3.92       'OIL'  3*  NO /
/


WCONPROD
     'BAN'      'OPEN'      'ORAT'      0.000      0.000      0.000  5* /
/


DATES             -- 5
 10  AUG 2007 /
/

WCONINJH
     'BAN'      'WATER'      1*      0 /
/

DATES             -- 6
 10  SEP 2007 /
/

WELOPEN
 'BAN' OPEN /
/

DATES             -- 7
 10  NOV 2007 /
/

WCONINJH
     'BAN'      'WATER'      1*      1.0 /
/
)";

std::string GCONSALE_deck = R"(
START             -- 0
10 MAI 2007 /
SCHEDULE

GRUPTREE
   'G1'  'FIELD' /
   'G2'  'FIELD' /
   'G4'  'FIELD' /
   'G5'  'FIELD' /
/

GCONSALE
'G1' 50000 55000 45000 WELL /
/

GECON
 'G1'  1*  200000.0  /
 'G2'  1*  200000.0  /
/

GCONSUMP
'G1' 20 50 'a_node' /
'G2' 30 60 /
/

GSATPROD
'G4' 20 /
'G5' 30 /
/
DATES             -- 1
 10  JUN 2007 /
/

DATES             -- 2
 15  JUN 2007 /
/

DATES             -- 3
 20  JUN 2007 /
/

GRUPTREE
   'G3'  'G2' /
/

GCONSALE
'G1' 12345 12345 12345 WELL /
/

GCONSUMP
'G1' 10 77 'b_node' /
'G2' 10 77 /
/

GSATPROD
'G4' 40 /
'G5' 60 /
/

DATES             -- 4
 10  JUL 2007 /
/

DATES             -- 5
 10  AUG 2007 /
/

DATES             -- 6
 10  SEP 2007 /
/

DATES             -- 7
 10  NOV 2007 /
/
)";

std::string VFP_deck1 = R"(
START             -- 0
10 MAI 2007 /
SCHEDULE

VFPINJ
5  32.9   WAT   THP METRIC   BHP /
1 3 5 /
7 11 /
1 1.5 2.5 3.5 /
2 4.5 5.5 6.5 /

DATES             -- 1
 10  JUN 2007 /
/

DATES             -- 2
 15  JUN 2007 /
/

DATES             -- 3
 20  JUN 2007 /
/

VFPINJ
5  32.9   WAT   THP METRIC   BHP /
1 3 5 /
7 11 /
1 1.5 2.5 3.4 /
2 4.5 5.5 6.4 /

DATES             -- 4
 10  JUL 2007 /
/

DATES             -- 5
 10  AUG 2007 /
/

DATES             -- 6
 10  SEP 2007 /
/

DATES             -- 7
 10  NOV 2007 /
/
)";

namespace {

Schedule make_schedule(const std::string& deck_string)
{
    const auto deck = Parser{}.parseString(deck_string);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);
    return {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(SerializeWTest)
{
    auto sched = make_schedule(WTEST_deck);
    Opm::Schedule sched0;

    auto wtest1 = sched[0].wtest_config();
    auto wtest2 = sched[3].wtest_config();

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }

    BOOST_CHECK( wtest1 == sched0[0].wtest_config());
    BOOST_CHECK( wtest1 == sched0[1].wtest_config());
    BOOST_CHECK( wtest1 == sched0[2].wtest_config());

    BOOST_CHECK( wtest2 == sched0[3].wtest_config());
    BOOST_CHECK( wtest2 == sched0[4].wtest_config());
    BOOST_CHECK( wtest2 == sched0[5].wtest_config());
}

BOOST_AUTO_TEST_CASE(SerializeWList)
{
    auto sched = make_schedule(WTEST_deck);
    Opm::Schedule sched0;

    auto wlm1 = sched[0].wlist_manager();
    auto wlm2 = sched[3].wlist_manager();

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }

    BOOST_CHECK( wlm1 == sched0[0].wlist_manager());
    BOOST_CHECK( wlm1 == sched0[1].wlist_manager());
    BOOST_CHECK( wlm1 == sched0[2].wlist_manager());

    BOOST_CHECK( wlm2 == sched0[3].wlist_manager());
    BOOST_CHECK( wlm2 == sched0[4].wlist_manager());
    BOOST_CHECK( wlm2 == sched0[5].wlist_manager());
}

BOOST_AUTO_TEST_CASE(SerializeGECON)
{
    auto sched  = make_schedule(GCONSALE_deck);
    Opm::Schedule sched0;
    auto gecon1 = sched[0].gecon.get();

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }
    BOOST_CHECK( gecon1 == sched0[0].gecon());
    BOOST_CHECK( gecon1 == sched0[1].gecon());
}

BOOST_AUTO_TEST_CASE(SerializeGCONSALE)
{
    auto sched  = make_schedule(GCONSALE_deck);
    Opm::Schedule sched0;
    auto gconsale1 = sched[0].gconsale.get();
    auto gconsale2 = sched[3].gconsale.get();

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }

    BOOST_CHECK( gconsale1 == sched0[0].gconsale());
    BOOST_CHECK( gconsale1 == sched0[1].gconsale());
    BOOST_CHECK( gconsale1 == sched0[2].gconsale());

    BOOST_CHECK( gconsale2 == sched0[3].gconsale());
    BOOST_CHECK( gconsale2 == sched0[4].gconsale());
    BOOST_CHECK( gconsale2 == sched0[5].gconsale());
}

BOOST_AUTO_TEST_CASE(SerializeGCONSUMP)
{
    auto sched  = make_schedule(GCONSALE_deck);
    Opm::Schedule sched0;
    auto gconsump1 = sched[0].gconsump.get();
    auto gconsump2 = sched[3].gconsump.get();

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }

    BOOST_CHECK( gconsump1 == sched0[0].gconsump());
    BOOST_CHECK( gconsump1 == sched0[1].gconsump());
    BOOST_CHECK( gconsump1 == sched0[2].gconsump());

    BOOST_CHECK( gconsump2 == sched0[3].gconsump());
    BOOST_CHECK( gconsump2 == sched0[4].gconsump());
    BOOST_CHECK( gconsump2 == sched0[5].gconsump());
}

BOOST_AUTO_TEST_CASE(SerializeGSatProd) {
    auto sched  = make_schedule(GCONSALE_deck);
    Opm::Schedule sched0;
    auto gsatprod1 = sched[0].gsatprod.get();
    auto gsatprod2 = sched[3].gsatprod.get();

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }

    BOOST_CHECK( gsatprod1 == sched0[0].gsatprod());
    BOOST_CHECK( gsatprod1 == sched0[1].gsatprod());
    BOOST_CHECK( gsatprod1 == sched0[2].gsatprod());

    BOOST_CHECK( gsatprod2 == sched0[3].gsatprod());
    BOOST_CHECK( gsatprod2 == sched0[4].gsatprod());
    BOOST_CHECK( gsatprod2 == sched0[5].gsatprod());
}

BOOST_AUTO_TEST_CASE(SerializeVFP) {
    auto sched = make_schedule(VFP_deck1);
    Opm::Schedule sched0;
    auto vfpinj1 = sched[0].vfpinj;
    auto vfpinj2 = sched[3].vfpinj;

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }

    BOOST_CHECK( vfpinj1 == sched0[0].vfpinj);
    BOOST_CHECK( vfpinj1 == sched0[1].vfpinj);
    BOOST_CHECK( vfpinj1 == sched0[2].vfpinj);

    BOOST_CHECK( vfpinj2 == sched0[3].vfpinj);
    BOOST_CHECK( vfpinj2 == sched0[4].vfpinj);
    BOOST_CHECK( vfpinj2 == sched0[5].vfpinj);
}

BOOST_AUTO_TEST_CASE(SerializeGROUPS)
{
    auto sched = make_schedule(GCONSALE_deck);
    Opm::Schedule sched0;
    auto groups1 = sched[0].groups;
    auto groups2 = sched[3].groups;

    {
        Opm::Serialization::MemPacker packer;
        Opm::Serializer ser(packer);
        ser.pack(sched);
        ser.unpack(sched0);
    }

    BOOST_CHECK( groups1 == sched0[0].groups);
    BOOST_CHECK( groups1 == sched0[1].groups);
    BOOST_CHECK( groups1 == sched0[2].groups);

    BOOST_CHECK( groups2 == sched0[3].groups);
    BOOST_CHECK( groups2 == sched0[4].groups);
    BOOST_CHECK( groups2 == sched0[5].groups);
}
