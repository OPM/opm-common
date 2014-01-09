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
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckDoubleItem.hpp>
#include <opm/parser/eclipse/Utility/SimpleTable.hpp>

#define BOOST_TEST_MODULE SimpleTableTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/filesystem.hpp>

#include <stdexcept>
#include <iostream>

BOOST_AUTO_TEST_CASE(CreateSimpleTable_InvalidDeck) {
    Opm::DeckKeywordPtr keyword(new Opm::DeckKeyword("SWOF"));
    Opm::DeckRecordPtr record(new Opm::DeckRecord());
    Opm::DeckDoubleItemPtr item = Opm::DeckDoubleItemPtr(new Opm::DeckDoubleItem(/*name=*/"foo"));

    item->push_back(1);
    item->push_back(2);
    item->push_back(3);

    record->addItem(item);
    keyword->addRecord(record);

    std::vector<std::string> columnNames{"SW", "KRW", "KROW", "PCOW"};

    BOOST_CHECK_THROW(Opm::SimpleTable(keyword, columnNames, /*recordIdx=*/0) , std::runtime_error);
}

