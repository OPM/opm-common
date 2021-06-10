/*
  Copyright 2021 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms
  of the GNU General Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your option) any later
  version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#define BOOST_TEST_MODULE TEST_DEBUG_CONFIG
#include <boost/test/unit_test.hpp>

#include <string>

#include <opm/common/utility/DebugConfig.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

#include <opm/parser/eclipse/EclipseState/Runspec.hpp>


BOOST_AUTO_TEST_CASE(DEBUG_CONFIG1)
{
    Opm::DebugConfig dbg_config;
    BOOST_CHECK( dbg_config.update("UNKNOWN_KEY", "ON") );
    BOOST_CHECK( dbg_config("UNKNOWN_KEY") );
    BOOST_CHECK( !dbg_config("VERY_UNKNOWN_KEY") );


    BOOST_CHECK( dbg_config.update("WELLS", "ON") );
    BOOST_CHECK( dbg_config.update("WellS", "ON") );
    BOOST_CHECK( !dbg_config.update("WellS", "Jepp") );

    BOOST_CHECK( dbg_config[Opm::DebugConfig::Topic::WELLS] == Opm::DebugConfig::Verbosity::NORMAL );
    BOOST_CHECK( dbg_config(Opm::DebugConfig::Topic::WELLS) );

    BOOST_CHECK( !dbg_config(Opm::DebugConfig::Topic::INIT) );

    dbg_config.update("INIT");
    BOOST_CHECK( dbg_config[Opm::DebugConfig::Topic::INIT] == Opm::DebugConfig::Verbosity::NORMAL );
}


BOOST_AUTO_TEST_CASE(DEBUG_CONFIG2)
{
    std::string deck1 = R"(
DEBUGF
   WELLS=1 INIT=SILENT GROUP_CONTROL=1 UNKNOWN1=XYZ RESTART=ON/

)";

    std::string deck2 = R"(
DEBUGF
/
)";
    Opm::DebugConfig dbg_config;
    {
        auto deck = Opm::Parser{}.parseString(deck1);
        Opm::Runspec::update_debug_config(dbg_config, deck.getKeyword("DEBUGF"));
    }
    BOOST_CHECK(!dbg_config(Opm::DebugConfig::Topic::INIT));
    BOOST_CHECK(dbg_config(Opm::DebugConfig::Topic::WELLS));
    BOOST_CHECK(dbg_config("RESTART"));
    {
        auto deck = Opm::Parser{}.parseString(deck2);
        Opm::Runspec::update_debug_config(dbg_config, deck.getKeyword("DEBUGF"));
    }
    BOOST_CHECK(!dbg_config(Opm::DebugConfig::Topic::WELLS));
    BOOST_CHECK(!dbg_config("RESTART"));
}
