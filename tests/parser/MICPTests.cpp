/*
  Copyright 2021 NORCE.

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

#define BOOST_TEST_MODULE MICPTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/BiofilmTable.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

using namespace Opm;

BOOST_AUTO_TEST_CASE( TestMICP ) {
    const char *data =
      "RUNSPEC\n"
      "WATER\n"
      "MICP\n";

    auto deck = Parser().parseString(data);

    Runspec runspec( deck );
    const auto& phases = runspec.phases();
    BOOST_CHECK_EQUAL( 1U, phases.size() );
    BOOST_CHECK( phases.active( Phase::WATER ) );
    BOOST_CHECK( runspec.micp() );
}

BOOST_AUTO_TEST_CASE( TestBiofPara ) {
    const char *data =
    "DIMENS\n"
    "10 10 10 /\n"
    "TABDIMS\n"
    "2 /\n"
    "GRID\n"
    "DX\n"
    "1000*0.25 /\n"
    "DY\n"
    "1000*0.25 /\n"
    "DZ\n"
    "1000*0.25 /\n"
    "TOPS\n"
    "100*0.25 /\n"
    "PROPS\n"
    "BIOFPARA\n"
    " 1. 2. 3. 4. 5. 6. 7. 8. 9. 10. 11. 12. 13. /\n"
    "/\n";

    UnitSystem unitSystem = UnitSystem( UnitSystem::UnitType::UNIT_TYPE_METRIC );

    auto deck = Parser().parseString(data);

    double siFactor1 = unitSystem.parse("1/Time").getSIScaling();

    TableManager tables(deck);
    const auto& biofilmTables = tables.getBiofilmTables();
    const BiofilmTable& biofilmTable = biofilmTables.getTable<BiofilmTable>(0);
    BOOST_CHECK_EQUAL( biofilmTable.getDensityBiofilm().front()                , 1.  );
    BOOST_CHECK_EQUAL( biofilmTable.getMicrobialDeathRate().front()            , 2.  * siFactor1 );
    BOOST_CHECK_EQUAL( biofilmTable.getMaximumGrowthRate().front()             , 3.  * siFactor1 );
    BOOST_CHECK_EQUAL( biofilmTable.getHalfVelocityOxygen().front()            , 4.  );
    BOOST_CHECK_EQUAL( biofilmTable.getYieldGrowthCoefficient().front()        , 5.  );
    BOOST_CHECK_EQUAL( biofilmTable.getOxygenConsumptionFactor().front()       , 6.  );
    BOOST_CHECK_EQUAL( biofilmTable.getMicrobialAttachmentRate().front()       , 7.  * siFactor1 );
    BOOST_CHECK_EQUAL( biofilmTable.getDetachmentRate().front()                , 8.  * siFactor1 );
    BOOST_CHECK_EQUAL( biofilmTable.getDetachmentExponent().front()            , 9.  );
    BOOST_CHECK_EQUAL( biofilmTable.getMaximumUreaUtilization().front()        , 10. * siFactor1 );
    BOOST_CHECK_EQUAL( biofilmTable.getHalfVelocityUrea().front()              , 11. );
    BOOST_CHECK_EQUAL( biofilmTable.getDensityCalcite().front()                , 12. );
    BOOST_CHECK_EQUAL( biofilmTable.getYieldUreaToCalciteCoefficient().front() , 13. );
}
