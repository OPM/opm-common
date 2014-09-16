/*
  Copyright (C) 2013 by Andreas Lauser

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

#define BOOST_TEST_MODULE ParserIntegrationTests
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Utility/PvtoTable.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserDoubleItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>
#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

using namespace Opm;

const char *pvtoData = "\n\
TABDIMS\n\
-- NTSFUN NTPVT NSSFUN NPPVT NTFIP NRPVT\n\
     1      2     30    24    10    20  /\n\
\n\
PVTO\n\
--   Rs       PO           BO           MUO\n\
     1e-3     1            1.01         1.02\n\
              250          1.15         0.95\n\
              500          1.20         0.93 /\n\
     1e-2     14.8         1.05         1.03\n\
              251          1.25         0.98\n\
              502          1.30         0.95 /\n\
/\n\
     1e-1     1.1          1.02         1.03\n\
              253          1.16         0.96\n\
              504          1.21         0.97 /\n\
     1e00     15           1.06         1.04\n\
              255          1.26         0.99\n\
              506          1.31         0.96 /\n\
/\n";


static void check_parser(ParserPtr parser) {
    DeckPtr deck =  parser->parseString(pvtoData);
    DeckKeywordConstPtr kw1 = deck->getKeyword("PVTO" , 0);
    BOOST_CHECK_EQUAL(5U , kw1->size());

    DeckRecordConstPtr record0 = kw1->getRecord(0);
    DeckRecordConstPtr record1 = kw1->getRecord(1);
    DeckRecordConstPtr record2 = kw1->getRecord(2);
    DeckRecordConstPtr record3 = kw1->getRecord(3);
    DeckRecordConstPtr record4 = kw1->getRecord(4);

    DeckItemConstPtr item0_0 = record0->getItem("RS");
    DeckItemConstPtr item0_1 = record0->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item0_0->size());
    BOOST_CHECK_EQUAL(9U , item0_1->size());
    BOOST_CHECK_EQUAL(2U , record0->size());

    DeckItemConstPtr item1_0 = record1->getItem("RS");
    DeckItemConstPtr item1_1 = record1->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item1_0->size());
    BOOST_CHECK_EQUAL(9U , item1_1->size());
    BOOST_CHECK_EQUAL(2U , record1->size());

    DeckItemConstPtr item2_0 = record2->getItem("RS");
    DeckItemConstPtr item2_1 = record2->getItem("DATA");
    BOOST_CHECK(item2_0->defaultApplied(0));
    BOOST_CHECK_EQUAL(0U , item2_1->size());
    BOOST_CHECK_EQUAL(2U , record2->size());

    DeckItemConstPtr item3_0 = record3->getItem("RS");
    DeckItemConstPtr item3_1 = record3->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item3_0->size());
    BOOST_CHECK_EQUAL(9U , item3_1->size());
    BOOST_CHECK_EQUAL(2U , record3->size());

    DeckItemConstPtr item4_0 = record4->getItem("RS");
    DeckItemConstPtr item4_1 = record4->getItem("DATA");
    BOOST_CHECK_EQUAL(1U , item4_0->size());
    BOOST_CHECK_EQUAL(9U , item4_1->size());
    BOOST_CHECK_EQUAL(2U , record4->size());

    Opm::PvtoTable pvtoTable;
    pvtoTable.init(kw1, /*tableIdx=*/0);
    const auto &outerTable = *pvtoTable.getOuterTable();
    const auto &innerTable0 = *pvtoTable.getInnerTable(0);

    BOOST_CHECK_EQUAL(2, outerTable.numRows());
    BOOST_CHECK_EQUAL(4, outerTable.numColumns());
    BOOST_CHECK_EQUAL(3, innerTable0.numRows());
    BOOST_CHECK_EQUAL(3, innerTable0.numColumns());

    BOOST_CHECK_EQUAL(1e-3, outerTable.getGasSolubilityColumn()[0]);
    BOOST_CHECK_EQUAL(1.0e5, outerTable.getPressureColumn()[0]);
    BOOST_CHECK_EQUAL(outerTable.getPressureColumn()[0], innerTable0.getPressureColumn()[0]);
    BOOST_CHECK_EQUAL(1.01, outerTable.getOilFormationFactorColumn()[0]);
    BOOST_CHECK_EQUAL(outerTable.getOilFormationFactorColumn()[0], innerTable0.getOilFormationFactorColumn()[0]);
    BOOST_CHECK_EQUAL(1.02e-3, outerTable.getOilViscosityColumn()[0]);
    BOOST_CHECK_EQUAL(outerTable.getOilViscosityColumn()[0], innerTable0.getOilViscosityColumn()[0]);
}


BOOST_AUTO_TEST_CASE( parse_PVTO_OK ) {
    ParserPtr parser(new Parser());
    check_parser( parser );
}
