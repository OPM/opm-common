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


#define BOOST_TEST_MODULE LogTests

#include <stdexcept>
#include <iostream>
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>
#include <opm/parser/eclipse/OpmLog/LogBackend.hpp>
#include <opm/parser/eclipse/OpmLog/MessageCounter.hpp>
#include <opm/parser/eclipse/OpmLog/StreamLog.hpp>
#include <opm/parser/eclipse/OpmLog/LogUtil.hpp>

using namespace Opm;


BOOST_AUTO_TEST_CASE(DoLogging) {
    OpmLog::addMessage(Log::MessageType::Warning , "Warning1");
    OpmLog::addMessage(Log::MessageType::Warning , "Warning2");
}


BOOST_AUTO_TEST_CASE(Test_Format) {
    BOOST_CHECK_EQUAL( "/path/to/file:100: There is a mild fuckup here?" , Log::fileMessage("/path/to/file" , 100 , "There is a mild fuckup here?"));

    BOOST_CHECK_EQUAL( "error: This is the error" ,     Log::prefixMessage(Log::MessageType::Error , "This is the error"));
    BOOST_CHECK_EQUAL( "warning: This is the warning" , Log::prefixMessage(Log::MessageType::Warning , "This is the warning"));
    BOOST_CHECK_EQUAL( "note: This is the note" ,       Log::prefixMessage(Log::MessageType::Note , "This is the note"));
}



BOOST_AUTO_TEST_CASE(Test_AbstractBackend) {
    int64_t mask = 1+4+16;
    LogBackend backend(mask);

    BOOST_CHECK_EQUAL(false , backend.includeMessage(0 ));
    BOOST_CHECK_EQUAL(true  , backend.includeMessage(1 ));
    BOOST_CHECK_EQUAL(false , backend.includeMessage(2 ));
    BOOST_CHECK_EQUAL(true  , backend.includeMessage(4 ));
    BOOST_CHECK_EQUAL(false , backend.includeMessage(8 ));
    BOOST_CHECK_EQUAL(true  , backend.includeMessage(16 ));

    BOOST_CHECK_EQUAL(false, backend.includeMessage(6 ));
    BOOST_CHECK_EQUAL(true , backend.includeMessage(5 ));
}



BOOST_AUTO_TEST_CASE(Test_Logger) {
    Logger logger;
    std::ostringstream log_stream;
    std::shared_ptr<MessageCounter> counter = std::make_shared<MessageCounter>();
    std::shared_ptr<StreamLog> streamLog = std::make_shared<StreamLog>( log_stream , Log::MessageType::Warning );
    BOOST_CHECK_EQUAL( false , logger.hasBackend("NO"));

    logger.addBackend("COUNTER" , counter);
    logger.addBackend("STREAM" , streamLog);
    BOOST_CHECK_EQUAL( true , logger.hasBackend("COUNTER"));
    BOOST_CHECK_EQUAL( true , logger.hasBackend("STREAM"));

    logger.addMessage( Log::MessageType::Error , "Error");
    logger.addMessage( Log::MessageType::Warning , "Warning");
    BOOST_CHECK_EQUAL( 1 , counter->numWarnings() );
    BOOST_CHECK_EQUAL( 1 , counter->numErrors() );
    BOOST_CHECK_EQUAL( 0 , counter->numNotes() );

    BOOST_CHECK_EQUAL( log_stream.str() , "Warning\n");
}


BOOST_AUTO_TEST_CASE(LoggerAddTypes_PowerOf2) {
    Logger logger;
    int64_t not_power_of2 = 13;
    int64_t power_of2 = 4096;

    BOOST_CHECK_THROW( logger.addMessageType( not_power_of2 , "Prefix") , std::invalid_argument);
    BOOST_CHECK_THROW( logger.enabledMessageType( not_power_of2 ) , std::invalid_argument);

    logger.addMessageType( power_of2 , "Prefix");
    BOOST_CHECK( logger.enabledMessageType( power_of2 ));
    BOOST_CHECK_EQUAL( false , logger.enabledMessageType( 2*power_of2 ));
}



BOOST_AUTO_TEST_CASE(LoggerDefaultTypesEnabled) {
    Logger logger;
    BOOST_CHECK( logger.enabledMessageType( Log::MessageType::Error ));
    BOOST_CHECK( logger.enabledMessageType( Log::MessageType::Warning ));
    BOOST_CHECK( logger.enabledMessageType( Log::MessageType::Note ));
}
