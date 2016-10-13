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
#include <boost/algorithm/string/join.hpp>
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>

using namespace Opm;




BOOST_AUTO_TEST_CASE( parse_TITLE_OK ) {
    Parser parser;
    std::string fileWithTitleKeyword("testdata/integration_tests/TITLE/TITLE1.txt");

    auto deck = parser.parseFile(fileWithTitleKeyword, ParseContext());

    BOOST_CHECK_EQUAL(size_t(2), deck.size());
    BOOST_CHECK_EQUAL (true, deck.hasKeyword("TITLE"));

    const auto& titleKeyword = deck.getKeyword("TITLE");
    const auto& record = titleKeyword.getRecord(0);
    const auto& item = record.getItem(0);

    std::vector<std::string> itemValue = item.getData< std::string >();
    std::string itemValueString = boost::algorithm::join(itemValue, " ");

    BOOST_CHECK_EQUAL (0, itemValueString.compare("This is the title of the model."));
    BOOST_CHECK_EQUAL (true, deck.hasKeyword("START"));
}
