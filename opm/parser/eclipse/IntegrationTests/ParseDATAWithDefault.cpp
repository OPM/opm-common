/*
  Copyright (C) 2014 Statoil ASE

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

#include <opm/parser/eclipse/Utility/PvtgTable.hpp>

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



const char *dataMissingRecord = "\n\
ENDSCALE\n\
     1*     1*     2 /\n\
\n\
ENKRVD\n\
100 1   2  3  4  5  6  7   200 11 22 33 44 55 66 77 /\n\
";



BOOST_AUTO_TEST_CASE( ParseMissingRECORD_THrows) {
    ParserPtr parser(new Parser());
    BOOST_CHECK_THROW( parser->parseString( dataMissingRecord ) , std::invalid_argument);
}




const char *data = "\n\
ENDSCALE\n\
     1*     1*     2 /\n\
\n\
ENKRVD\n\
100 *   2  3  4  5  6  7   200 11 22 33 44 55 66 77 /\n\
100 1   2  3  4  5  6  7   200 11 22 33 44 55 66 77 /\n\
";



BOOST_AUTO_TEST_CASE( parse_DATAWithDefult_OK ) {
    ParserPtr parser(new Parser());
    DeckConstPtr deck = parser->parseString( data );
    DeckKeywordConstPtr keyword = deck->getKeyword( "ENKRVD" );
    DeckRecordConstPtr rec0 = keyword->getRecord(0);
    DeckRecordConstPtr rec1 = keyword->getRecord(1);
    
    DeckItemConstPtr item0 = rec0->getItem(0);
    DeckItemConstPtr item1 = rec1->getItem(0);
    
    BOOST_CHECK_EQUAL( 2U , keyword->size());

    BOOST_CHECK_THROW( item0->defaultApplied() , std::invalid_argument);
}



