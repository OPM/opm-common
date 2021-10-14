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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/MICPpara.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>

BOOST_AUTO_TEST_CASE(TestMICP) {

    const char *data =
      "RUNSPEC\n"
      "WATER\n"
      "MICP\n";

    Opm::Parser parser;

    auto deck = parser.parseString(data);

    Opm::Runspec runspec( deck );
    const auto& phases = runspec.phases();
    BOOST_CHECK_EQUAL( 1U, phases.size() );
    BOOST_CHECK( phases.active( Opm::Phase::WATER ) );
    BOOST_CHECK( runspec.micp() );
}

BOOST_AUTO_TEST_CASE( TestMICPPARA ) {
    const char *data =
        "MICPPARA\n"
        " 1. 2. 3. 4. 5. 6. 7. 8. 9. 10. 11. 12. 13. 14. 15. 16. 17. /\n";

    Opm::Parser parser;

    auto deck = parser.parseString(data);

    Opm::EclipseState eclipsestate( deck );
    const auto& MICPpara = eclipsestate.getMICPpara();
    BOOST_CHECK_EQUAL( MICPpara.getDensityBiofilm()             , 1.  );
    BOOST_CHECK_EQUAL( MICPpara.getDensityCalcite()             , 2.  );
    BOOST_CHECK_EQUAL( MICPpara.getDetachmentRate()             , 3.  );
    BOOST_CHECK_EQUAL( MICPpara.getCriticalPorosity()           , 4.  );
    BOOST_CHECK_EQUAL( MICPpara.getFittingFactor()              , 5.  );
    BOOST_CHECK_EQUAL( MICPpara.getHalfVelocityOxygen()         , 6.  );
    BOOST_CHECK_EQUAL( MICPpara.getHalfVelocityUrea()           , 7.  );
    BOOST_CHECK_EQUAL( MICPpara.getMaximumGrowthRate()          , 8.  );
    BOOST_CHECK_EQUAL( MICPpara.getMaximumUreaUtilization()     , 9.  );
    BOOST_CHECK_EQUAL( MICPpara.getMicrobialAttachmentRate()    , 10. );
    BOOST_CHECK_EQUAL( MICPpara.getMicrobialDeathRate()         , 11. );
    BOOST_CHECK_EQUAL( MICPpara.getMinimumPermeability()        , 12. );
    BOOST_CHECK_EQUAL( MICPpara.getOxygenConsumptionFactor()    , 13. );
    BOOST_CHECK_EQUAL( MICPpara.getYieldGrowthCoefficient()     , 14. );
    BOOST_CHECK_EQUAL( MICPpara.getMaximumOxygenConcentration() , 15. );
    BOOST_CHECK_EQUAL( MICPpara.getMaximumUreaConcentration()   , 16. );
    BOOST_CHECK_EQUAL( MICPpara.getToleranceBeforeClogging()    , 17. );
}
