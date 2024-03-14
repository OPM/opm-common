/*
Copyright (C) 2024 SINTEF Digital, Mathematics and Cybernetics.

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

#define BOOST_TEST_MODULE Compositional

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <initializer_list>
#include <stdexcept>

using namespace Opm;

namespace {

Deck createCompositionalDeck()
{
    return Parser{}.parseString(R"(
------------------------------------------------------------------------
RUNSPEC
------------------------------------------------------------------------
TITLE
   SIMPLE CO2 CASE FOR PARSING TEST

METRIC

TABDIMS
8* 2 3/

COMPS
3 /

------------------------------------------------------------------------
PROPS
------------------------------------------------------------------------

CNAMES
DECANE
CO2
METHANE
/

ROCK
68 0 /

EOS
PR /
SRK /

BIC
0
1 2 /
1
2 3 /

ACF
0.4 0.2 0.01 /
0.5 0.3 0.03 /

PCRIT
20. 70. 40. /
21. 71. 41. /

TCRIT
600. 300. 190. /
601. 301. 191. /

MW
142.  44.  16. /
142.1 44.1 16.1 /

VCRIT
0.6  0.1  0.1 /
0.61 0.11 0.11 /


STCOND
15.0 /

END
)");
}

void check_vectors_close(const std::vector<double>& v1, const std::vector<double>& v2, double tolerance) {
    BOOST_CHECK_EQUAL(v1.size(), v2.size());
    for(size_t i = 0; i < v1.size(); ++i) {
        BOOST_CHECK_CLOSE(v1[i], v2[i], tolerance);
    }
}

BOOST_AUTO_TEST_CASE(CompositionalParsingTest) {
    const Deck deck = createCompositionalDeck();
    const Runspec runspec{deck};
    const Tabdims tabdims {deck};
    const CompositionalConfig comp_config{deck, runspec};

    constexpr std::size_t num_comps = 3;
    constexpr std::size_t num_eos_res = 2;
    constexpr std::size_t num_eos_sur = 3;

    BOOST_CHECK(runspec.compositionalMode());
    BOOST_CHECK_EQUAL(num_comps, runspec.numComps());

    BOOST_CHECK_EQUAL(num_eos_res, tabdims.getNumEosRes());
    BOOST_CHECK_EQUAL(num_eos_sur, tabdims.getNumEosSur());

    BOOST_CHECK(CompositionalConfig::EOSType::PR == comp_config.eosType(0));
    BOOST_CHECK(CompositionalConfig::EOSType::SRK == comp_config.eosType(1));

    BOOST_CHECK_EQUAL(288.15, comp_config.standardTemperature());
    BOOST_CHECK_EQUAL(101325, comp_config.standardPressure());

    BOOST_CHECK_EQUAL(num_comps, comp_config.compName().size());
    BOOST_CHECK("DECANE" == comp_config.compName()[0]);
    BOOST_CHECK("CO2" == comp_config.compName()[1]);
    BOOST_CHECK("METHANE" == comp_config.compName()[2]);

    constexpr double tolerance = 1.e-5;
    const auto& usys = deck.getActiveUnitSystem();
    using M = UnitSystem::measure;

    {
        const auto& acf0 = comp_config.acentricFactors(0);
        BOOST_CHECK_EQUAL(num_comps, acf0.size());
        check_vectors_close(std::vector<double>{0.4, 0.2, 0.01}, acf0, tolerance);
        const auto& acf1 = comp_config.acentricFactors(1);
        BOOST_CHECK_EQUAL(num_comps, acf1.size());
        check_vectors_close(std::vector<double>{0.5, 0.3, 0.03}, acf1, tolerance);
    }

    {
        const auto& cp0 = comp_config.criticalPressure(0);
        BOOST_CHECK_EQUAL(num_comps, cp0.size());
        const std::vector<double> ref_cp0 = {usys.to_si(M::pressure, 20), usys.to_si(M::pressure, 70), usys.to_si(M::pressure, 40)};
        check_vectors_close(ref_cp0, cp0, tolerance);
        const auto& cp1 = comp_config.criticalPressure(1);
        BOOST_CHECK_EQUAL(num_comps, cp1.size());
        const std::vector<double> ref_cp1 = {usys.to_si(M::pressure, 21), usys.to_si(M::pressure, 71), usys.to_si(M::pressure, 41)};
        check_vectors_close(ref_cp1, cp1, tolerance);
    }

    {
        const auto& ct0 = comp_config.criticalTemperature(0);
        BOOST_CHECK_EQUAL(num_comps, ct0.size());
        const std::vector<double> ref_ct0 = {usys.to_si(M::temperature, 600), usys.to_si(M::temperature, 300), usys.to_si(M::temperature, 190)};
        check_vectors_close(ref_ct0, ct0, tolerance);
        const auto& ct1 = comp_config.criticalTemperature(1);
        const std::vector<double> ref_ct1 = {usys.to_si(M::temperature, 601), usys.to_si(M::temperature, 301), usys.to_si(M::temperature, 191)};
        check_vectors_close(ref_ct1, ct1, tolerance);
    }

    {
        const auto& cv0 = comp_config.criticalVolume(0);
        BOOST_CHECK_EQUAL(num_comps, cv0.size());
        const std::vector<double> ref_cv0 { usys.to_si( "GeometricVolume/Moles", 0.6),
                                            usys.to_si( "GeometricVolume/Moles", 0.1),
                                            usys.to_si( "GeometricVolume/Moles", 0.1) };
        check_vectors_close(ref_cv0, cv0, tolerance);
        const auto& cv1 = comp_config.criticalVolume(1);
        BOOST_CHECK_EQUAL(num_comps, cv1.size());
        const std::vector<double> ref_cv1 { usys.to_si( "GeometricVolume/Moles", 0.61),
                                            usys.to_si( "GeometricVolume/Moles", 0.11),
                                            usys.to_si( "GeometricVolume/Moles", 0.11) };
        check_vectors_close(ref_cv1, cv1, tolerance);
    }

    {
        const auto& bic0 = comp_config.binaryInteractionCoefficient(0);
        constexpr std::size_t bic_size = num_comps * (num_comps - 1) / 2;
        BOOST_CHECK_EQUAL(bic_size, bic0.size());
        const std::vector<double> ref_bic0 {0, 1, 2};
        check_vectors_close(ref_bic0, bic0, tolerance);
        const auto& bic1 = comp_config.binaryInteractionCoefficient(1);
        BOOST_CHECK_EQUAL(bic_size, bic1.size());
        const std::vector<double> ref_bic1 {1, 2, 3};
        check_vectors_close(ref_bic1, bic1, tolerance);
    }

    {
        const auto& mw0 = comp_config.molecularWeights(0);
        BOOST_CHECK_EQUAL(num_comps, mw0.size());
        const std::vector<double> ref_mw0 { usys.to_si( "Mass/Moles", 142.),
                                            usys.to_si( "Mass/Moles", 44.),
                                            usys.to_si( "Mass/Moles", 16) };
        check_vectors_close(ref_mw0, mw0, tolerance);
        const auto& mw1 = comp_config.molecularWeights(1);
        BOOST_CHECK_EQUAL(num_comps, mw1.size());
        const std::vector<double> ref_mw1 { usys.to_si( "Mass/Moles", 142.1),
                                            usys.to_si( "Mass/Moles", 44.1),
                                            usys.to_si( "Mass/Moles", 16.1) };
        check_vectors_close(ref_mw1, mw1, tolerance);
    }
}

}
