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

OIL
GAS
DIMENS
4 1 1
/

COMPS
3 /

------------------------------------------------------------------------
GRID
------------------------------------------------------------------------
DX
4*10
/
DY
4*1
/
DZ
4*1
/

TOPS
4*0
/


PERMX
4*100
/

PERMY
4*100
/

PERMZ
4*100
/

PORO
1. 2*0.1  1.
/

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

SGOF
-- Sg    Krg    Kro    Pcgo
   0.0   0.0    1.0    0.0
   0.1   0.1    0.9    0.0
   0.2   0.2    0.8    0.0
   0.3   0.3    0.7    0.0
   0.4   0.4    0.6    0.0
   0.5   0.5    0.5    0.0
   0.6   0.6    0.4    0.0
   0.7   0.7    0.3    0.0
   0.8   0.8    0.2    0.0
   0.9   0.9    0.1    0.0
   1.0   1.0    0.0    0.0
/


------------------------------------------------------------------------
SOLUTION
------------------------------------------------------------------------

PRESSURE
1*150 2*75. 1*37.5
/

SGAS
4*1.
/

TEMPI
4*150
/

XMF
1*0.99 3*0.5
1*0.009 3*0.3
1*0.001 3*0.2
/

YMF
1*0.009 3*0.3
1*0.001 3*0.2
1*0.99  3*0.5
/

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
        const std::vector<double> ref_ct0 = {usys.to_si(M::temperature_absolute, 600), usys.to_si(M::temperature_absolute, 300), usys.to_si(M::temperature_absolute, 190)};
        check_vectors_close(ref_ct0, ct0, tolerance);
        const auto& ct1 = comp_config.criticalTemperature(1);
        const std::vector<double> ref_ct1 = {usys.to_si(M::temperature_absolute, 601), usys.to_si(M::temperature_absolute, 301), usys.to_si(M::temperature_absolute, 191)};
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

    EclipseState es(deck);
    const auto& fp = es.fieldProps();
    const std::size_t num_cell = es.getInputGrid().getNumActive();
    {
        const auto& xmf = fp.get_double("XMF");
        BOOST_CHECK_EQUAL(xmf.size(), num_comps * num_cell);
        std::vector<double> ref_xmf{0.99, 0.5, 0.5, 0.5,
                                    0.009, 0.3, 0.3, 0.3,
                                    0.001, 0.2, 0.2, 0.2};
        check_vectors_close(xmf, ref_xmf, tolerance);
    }

    {
        const auto& ymf = fp.get_double("YMF");
        BOOST_CHECK_EQUAL(ymf.size(), num_comps * num_cell);
        std::vector<double> ref_ymf{0.009, 0.3, 0.3, 0.3,
                                    0.001, 0.2, 0.2, 0.2,
                                    0.99, 0.5, 0.5, 0.5};

        check_vectors_close(ymf, ref_ymf, tolerance);
    }

    {
        const auto& tempi = fp.get_double("TEMPI");
        BOOST_CHECK_EQUAL(tempi.size(), num_cell);
        std::vector<double> ref_tempi(4, 150. + 273.15);
        check_vectors_close(tempi, ref_tempi, tolerance);
    }
}

Deck createCompositionalDeckZMF()
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

OIL
GAS
DIMENS
4 1 1
/

COMPS
3 /

------------------------------------------------------------------------
GRID
------------------------------------------------------------------------
DX
4*10
/
DY
4*1
/
DZ
4*1
/

TOPS
4*0
/


PERMX
4*100
/

PERMY
4*100
/

PERMZ
4*100
/

PORO
1. 2*0.1  1.
/

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

SGOF
-- Sg    Krg    Kro    Pcgo
   0.0   0.0    1.0    0.0
   0.1   0.1    0.9    0.0
   0.2   0.2    0.8    0.0
   0.3   0.3    0.7    0.0
   0.4   0.4    0.6    0.0
   0.5   0.5    0.5    0.0
   0.6   0.6    0.4    0.0
   0.7   0.7    0.3    0.0
   0.8   0.8    0.2    0.0
   0.9   0.9    0.1    0.0
   1.0   1.0    0.0    0.0
/


------------------------------------------------------------------------
SOLUTION
------------------------------------------------------------------------

PRESSURE
1*150 2*75. 1*37.5
/

SGAS
4*1.
/

TEMPI
4*150
/

ZMF
1*0.99 3*0.5
1*0.009 3*0.3
1*0.001 3*0.2
/

END
)");
}
// createCompositionalDeckZMF() is almost same with createCompositionalDeck(), except using ZMF instead of XMF and YMF.
// Later we can decide which deck to keep or modify the two decks to test different sets of compositiona setups.
// For now, we keep both two.
BOOST_AUTO_TEST_CASE(CompositionalParsingTestZMF) {
    const Deck deck = createCompositionalDeckZMF();
    const Runspec runspec{deck};
    const Tabdims tabdims{deck};
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
        const std::vector<double> ref_cp0 = {usys.to_si(M::pressure, 20), usys.to_si(M::pressure, 70),
                                             usys.to_si(M::pressure, 40)};
        check_vectors_close(ref_cp0, cp0, tolerance);
        const auto& cp1 = comp_config.criticalPressure(1);
        BOOST_CHECK_EQUAL(num_comps, cp1.size());
        const std::vector<double> ref_cp1 = {usys.to_si(M::pressure, 21), usys.to_si(M::pressure, 71),
                                             usys.to_si(M::pressure, 41)};
        check_vectors_close(ref_cp1, cp1, tolerance);
    }

    {
        const auto& ct0 = comp_config.criticalTemperature(0);
        BOOST_CHECK_EQUAL(num_comps, ct0.size());
        const std::vector<double> ref_ct0 = {usys.to_si(M::temperature_absolute, 600),
                                             usys.to_si(M::temperature_absolute, 300),
                                             usys.to_si(M::temperature_absolute, 190)};
        check_vectors_close(ref_ct0, ct0, tolerance);
        const auto& ct1 = comp_config.criticalTemperature(1);
        const std::vector<double> ref_ct1 = {usys.to_si(M::temperature_absolute, 601),
                                             usys.to_si(M::temperature_absolute, 301),
                                             usys.to_si(M::temperature_absolute, 191)};
        check_vectors_close(ref_ct1, ct1, tolerance);
    }

    {
        const auto& cv0 = comp_config.criticalVolume(0);
        BOOST_CHECK_EQUAL(num_comps, cv0.size());
        const std::vector<double> ref_cv0{usys.to_si("GeometricVolume/Moles", 0.6),
                                          usys.to_si("GeometricVolume/Moles", 0.1),
                                          usys.to_si("GeometricVolume/Moles", 0.1)};
        check_vectors_close(ref_cv0, cv0, tolerance);
        const auto& cv1 = comp_config.criticalVolume(1);
        BOOST_CHECK_EQUAL(num_comps, cv1.size());
        const std::vector<double> ref_cv1{usys.to_si("GeometricVolume/Moles", 0.61),
                                          usys.to_si("GeometricVolume/Moles", 0.11),
                                          usys.to_si("GeometricVolume/Moles", 0.11)};
        check_vectors_close(ref_cv1, cv1, tolerance);
    }

    {
        const auto& bic0 = comp_config.binaryInteractionCoefficient(0);
        constexpr std::size_t bic_size = num_comps * (num_comps - 1) / 2;
        BOOST_CHECK_EQUAL(bic_size, bic0.size());
        const std::vector<double> ref_bic0{0, 1, 2};
        check_vectors_close(ref_bic0, bic0, tolerance);
        const auto& bic1 = comp_config.binaryInteractionCoefficient(1);
        BOOST_CHECK_EQUAL(bic_size, bic1.size());
        const std::vector<double> ref_bic1{1, 2, 3};
        check_vectors_close(ref_bic1, bic1, tolerance);
    }

    {
        const auto& mw0 = comp_config.molecularWeights(0);
        BOOST_CHECK_EQUAL(num_comps, mw0.size());
        const std::vector<double> ref_mw0{usys.to_si("Mass/Moles", 142.),
                                          usys.to_si("Mass/Moles", 44.),
                                          usys.to_si("Mass/Moles", 16)};
        check_vectors_close(ref_mw0, mw0, tolerance);
        const auto& mw1 = comp_config.molecularWeights(1);
        BOOST_CHECK_EQUAL(num_comps, mw1.size());
        const std::vector<double> ref_mw1{usys.to_si("Mass/Moles", 142.1),
                                          usys.to_si("Mass/Moles", 44.1),
                                          usys.to_si("Mass/Moles", 16.1)};
        check_vectors_close(ref_mw1, mw1, tolerance);
    }

    EclipseState es(deck);
    const auto& fp = es.fieldProps();
    const std::size_t num_cell = es.getInputGrid().getNumActive();
    {
        const auto& zmf = fp.get_double("ZMF");
        BOOST_CHECK_EQUAL(zmf.size(), num_comps * num_cell);
        std::vector<double> ref_zmf{0.99, 0.5, 0.5, 0.5,
                                    0.009, 0.3, 0.3, 0.3,
                                    0.001, 0.2, 0.2, 0.2};
        check_vectors_close(zmf, ref_zmf, tolerance);
    }

    {
        const auto& tempi = fp.get_double("TEMPI");
        BOOST_CHECK_EQUAL(tempi.size(), num_cell);
        std::vector<double> ref_tempi(4, 150. + 273.15);
        check_vectors_close(tempi, ref_tempi, tolerance);
    }
}

}
