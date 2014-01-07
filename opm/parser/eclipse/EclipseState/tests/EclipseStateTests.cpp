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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE EclipseStateTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/Deck/DeckIntItem.hpp>
#include <opm/parser/eclipse/Deck/DeckStringItem.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

using namespace Opm;

DeckPtr createDeck() {
    DeckPtr deck(new Deck());
    DeckKeywordPtr scheduleKeyword(new DeckKeyword("SCHEDULE"));
    DeckKeywordPtr startKeyword(new DeckKeyword("START"));
    DeckRecordPtr  startRecord(new DeckRecord());
    DeckIntItemPtr    dayItem( new DeckIntItem("DAY") );
    DeckStringItemPtr monthItem(new DeckStringItem("MONTH") );
    DeckIntItemPtr    yearItem(new DeckIntItem("YEAR"));


    dayItem->push_back( 8 );
    monthItem->push_back( "MAR" );
    yearItem->push_back( 1998 );

    startRecord->addItem( dayItem );
    startRecord->addItem( monthItem );
    startRecord->addItem( yearItem );
    startKeyword->addRecord( startRecord );

    deck->addKeyword( startKeyword );
    deck->addKeyword( scheduleKeyword );

    return deck;
}



BOOST_AUTO_TEST_CASE(CreatSchedule) {
    DeckPtr deck = createDeck();
    EclipseState state(deck);
    ScheduleConstPtr schedule = state.getSchedule();
    
    BOOST_CHECK_EQUAL( schedule->getStartDate() , boost::gregorian::date(1998 , 3 , 8 ));
}

