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

#define BOOST_TEST_MODULE ParserTests
#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/RawDeck/RawRecord.hpp>

BOOST_AUTO_TEST_CASE(RawRecordGetRecordStringReturnsTrimmedString) {
    Opm::RawRecordPtr record(new Opm::RawRecord(" 'NODIR '  'REVERS'  1  20                                       /"));
    const std::string& recordString = record->getRecordString();
    BOOST_CHECK_EQUAL("'NODIR '  'REVERS'  1  20", recordString);
}

BOOST_AUTO_TEST_CASE(RawRecordGetRecordsCorrectElementsReturned) {
    Opm::RawRecordPtr record(new Opm::RawRecord(" 'NODIR '  'REVERS'  1  20                                       /"));

    const std::deque<std::string>& recordElements = record->getItems();
    BOOST_CHECK_EQUAL((unsigned) 4, recordElements.size());

    BOOST_CHECK_EQUAL("NODIR ", recordElements[0]);
    BOOST_CHECK_EQUAL("REVERS", recordElements[1]);
    BOOST_CHECK_EQUAL("1", recordElements[2]);
    BOOST_CHECK_EQUAL("20", recordElements[3]);
}

BOOST_AUTO_TEST_CASE(RawRecordIsCompleteRecordCompleteRecordReturnsTrue) {
    bool isComplete = Opm::RawRecord::isTerminatedRecordString("'NODIR '  'REVERS'  1  20                                       /");
    BOOST_CHECK_EQUAL(true, isComplete);
}

BOOST_AUTO_TEST_CASE(RawRecordIsCompleteRecordInCompleteRecordReturnsFalse) {
    bool isComplete = Opm::RawRecord::isTerminatedRecordString("'NODIR '  'REVERS'  1  20                                       ");
    BOOST_CHECK_EQUAL(false, isComplete);
    isComplete = Opm::RawRecord::isTerminatedRecordString("'NODIR '  'REVERS  1  20 /");
    BOOST_CHECK_EQUAL(false, isComplete);
}


BOOST_AUTO_TEST_CASE(Rawrecord_OperatorThis_OK) {
  Opm::RawRecord record(" 'NODIR '  'REVERS'  1  20  /");
  Opm::RawRecordPtr recordPtr(new Opm::RawRecord(" 'NODIR '  'REVERS'  1  20  /"));  

  BOOST_CHECK_EQUAL( "NODIR " , record[0]);
  BOOST_CHECK_EQUAL( "REVERS" , record[1]);
  BOOST_CHECK_EQUAL( "1" , record[2]);
  BOOST_CHECK_EQUAL( "20" , record[3]);

  BOOST_CHECK_EQUAL( "20" , (*recordPtr)[3]);

  BOOST_CHECK_THROW( record[4] , std::out_of_range);
}


BOOST_AUTO_TEST_CASE(Rawrecord_PushFront_OK) {
  Opm::RawRecordPtr record(new Opm::RawRecord(" 'NODIR '  'REVERS'  1  20  /"));
  record->push_front( "String2" );
  record->push_front( "String1" );
  
  
  BOOST_CHECK_EQUAL( "String1" , (*record)[0]);
  BOOST_CHECK_EQUAL( "String2" , (*record)[1]);
}
