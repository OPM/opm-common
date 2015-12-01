/*
  Copyright 2013 Statoil ASA.
  2015 IRIS AS
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

#define BOOST_TEST_MODULE ParseMiscible
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/SorwmisTable.hpp>

using namespace Opm;

const char *miscibleData = "\n\
MISCIBLE\n\
 2  3 /\n\
\n";

const char *miscibleTightData = "\n\
MISCIBLE\n\
 1  2 /\n\
\n";

const char *sorwmisData = "\n\
SORWMIS\n\
.00 .00 \n\
.50 .00 \n\
1.0 .00 /\n\
.00 .00 \n\
.30 .20 \n\
1.0 .80 /\n\
\n";

BOOST_AUTO_TEST_CASE( PARSE_SORWMIS) {
    ParserPtr parser(new Parser());
    char tooFewTables[10000];
    std::strcpy(tooFewTables,miscibleTightData);
    std::strcat(tooFewTables,sorwmisData);

    // missing miscible keyword
    BOOST_CHECK_THROW (parser->parseString(sorwmisData, ParseMode()), std::invalid_argument );

    //too many tables
    BOOST_CHECK_THROW( parser->parseString(tooFewTables, ParseMode()), std::invalid_argument)

    char data[10000];
    std::strcpy(data,miscibleData);
    std::strcat(data,sorwmisData);
    DeckPtr deck1 =  parser->parseString(data, ParseMode());

    DeckKeywordConstPtr sorwmis = deck1->getKeyword("SORWMIS");
    DeckKeywordConstPtr miscible = deck1->getKeyword("MISCIBLE");

    DeckRecordConstPtr miscible0 = miscible->getRecord(0);
    DeckRecordConstPtr sorwmis0 = sorwmis->getRecord(0);
    DeckRecordConstPtr sorwmis1 = sorwmis->getRecord(1);


    // test number of columns
    int ntmisc = miscible0->getItem(0)->getInt(0);
    Opm::SorwmisTable sorwmisTable0;
    sorwmisTable0.initFORUNITTESTONLY(sorwmis0->getItem(0));
    BOOST_CHECK_EQUAL(sorwmisTable0.numColumns(),ntmisc);

    // test table input 1
    BOOST_CHECK_EQUAL(3U, sorwmisTable0.getWaterSaturationColumn().size());
    BOOST_CHECK_EQUAL(1.0, sorwmisTable0.getWaterSaturationColumn()[2]);
    BOOST_CHECK_EQUAL(0.0, sorwmisTable0.getMiscibleResidualOilColumn()[2]);

    // test table input 2
    Opm::SorwmisTable sorwmisTable1;
    sorwmisTable1.initFORUNITTESTONLY(sorwmis1->getItem(0));
    BOOST_CHECK_EQUAL(sorwmisTable1.numColumns(),ntmisc);

    BOOST_CHECK_EQUAL(3U, sorwmisTable1.getWaterSaturationColumn().size());
    BOOST_CHECK_EQUAL(0.3, sorwmisTable1.getWaterSaturationColumn()[1]);
    BOOST_CHECK_EQUAL(0.8, sorwmisTable1.getMiscibleResidualOilColumn()[2]);
}
