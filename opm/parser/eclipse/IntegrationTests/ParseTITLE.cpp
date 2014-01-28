/*
  Copyright 2013 Statoil ASA.

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
#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserIntItem.hpp>
#include <opm/parser/eclipse/Parser/ParserStringItem.hpp>

#include <opm/parser/eclipse/Parser/ParserEnums.hpp>

using namespace Opm;




BOOST_AUTO_TEST_CASE( parse_TITLE_OK ) {
    ParserPtr parser(new Parser());
    boost::filesystem::path fileWithTitleKeyword("testdata/integration_tests/TITLE/TITLE1.txt");
    
    DeckPtr deck = parser->parseFile (fileWithTitleKeyword.string(), true);

    BOOST_CHECK_EQUAL(size_t(2), deck->size());
    BOOST_CHECK_EQUAL (true, deck->hasKeyword("TITLE"));
    DeckKeywordConstPtr titleKeyword = deck->getKeyword("TITLE");
    DeckRecordConstPtr record = titleKeyword->getRecord(0);
    DeckItemPtr item = record->getItem(0);
    std::string itemValue = item->getString(0);
    BOOST_CHECK (itemValue.length() > 0 ); //Should check for actual value?
    BOOST_CHECK_EQUAL (true, deck->hasKeyword("START"));


}


