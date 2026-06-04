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
#include <opm/common/utility/OpmInputError.hpp>

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

EOSS
PRCORR /
RK /
ZJ /

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

TBOIL
350. 450. 550. /
351. 451. 551. /

MW
142.  44.  16. /
142.1 44.1 16.1 /

VCRIT
0.6  0.1  0.1 /
0.61 0.11 0.11 /

SSHIFT
0.1 0.2 0.3 /
0.11 0.21 0.31 /


ZCRIT
0.28 0.25 0.26 /
0.29 0.26 0.27 /


BICS
0
0.1 0.2 /
0.05
0.15 0.25 /
0.06
0.16 0.26 /

ACFS
0.41 0.21 0.011 /
0.51 0.31 0.031 /
0.52 0.32 0.032 /

PCRITS
22. 72. 42. /
23. 73. 43. /
24. 74. 44. /

TCRITS
602. 302. 192. /
603. 303. 193. /
604. 304. 194. /

TBOILS
352. 452. 552. /
353. 453. 553. /
354. 454. 554. /

MWS
143.  45.  17. /
143.1 45.1 17.1 /
143.2 45.2 17.2 /

VCRITS
0.62 0.12 0.12 /
0.63 0.13 0.13 /
0.64 0.14 0.14 /

ZCRITS
0.30 0.27 0.28 /
0.31 0.28 0.29 /
0.32 0.29 0.30 /


SSHIFTS
0.12 0.22 0.32 /
0.13 0.23 0.33 /
0.14 0.24 0.34 /


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

Deck createCompositionalDeckWithoutDefaultedKeywords()
{
    return Parser{}.parseString(R"(
------------------------------------------------------------------------
RUNSPEC
------------------------------------------------------------------------
METRIC

TABDIMS
8* 2 3/

OIL
GAS

DIMENS
1 1 1 /

COMPS
3 /

------------------------------------------------------------------------
GRID
------------------------------------------------------------------------
DX
1 /
DY
1 /
DZ
1 /
TOPS
0 /
PERMX
100 /
PERMY
100 /
PERMZ
100 /
PORO
0.1 /

------------------------------------------------------------------------
PROPS
------------------------------------------------------------------------
CNAMES
DECANE
CO2
METHANE
/

EOS
PR /
SRK /

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

END
)");
}

void check_vectors_close(const std::vector<double>& v1, const std::vector<double>& v2, double tolerance) {
    BOOST_CHECK_EQUAL(v1.size(), v2.size());
    for(std::size_t i = 0; i < v1.size(); ++i) {
        BOOST_CHECK_CLOSE(v1[i], v2[i], tolerance);
    }
}

BOOST_AUTO_TEST_CASE(DefaultedCompositionalKeywordsArePopulatedWhenAbsent) {
    const Deck deck = createCompositionalDeckWithoutDefaultedKeywords();
    const Runspec runspec{deck};
    const CompositionalConfig comp_config{deck, runspec};

    constexpr std::size_t num_comps = 3;
    constexpr std::size_t num_eos_res = 2;
    constexpr std::size_t bic_size = num_comps * (num_comps - 1) / 2;
    constexpr double tolerance = 1.e-5;

    for (std::size_t eos_region = 0; eos_region < num_eos_res; ++eos_region) {
        const auto& bic = comp_config.binaryInteractionCoefficient(eos_region);
        BOOST_CHECK_EQUAL(bic_size, bic.size());
        check_vectors_close(std::vector<double>(bic_size, 0.0), bic, tolerance);

        const auto& volume_shift = comp_config.volumeShifts(eos_region);
        BOOST_CHECK_EQUAL(num_comps, volume_shift.size());
        check_vectors_close(std::vector<double>(num_comps, 0.0), volume_shift, tolerance);
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
        const auto& bt0 = comp_config.boilingTemperature(0);
        BOOST_CHECK_EQUAL(num_comps, bt0.size());
        const std::vector<double> ref_bt0 = {usys.to_si(M::temperature_absolute, 350), usys.to_si(M::temperature_absolute, 450), usys.to_si(M::temperature_absolute, 550)};
        check_vectors_close(ref_bt0, bt0, tolerance);
        const auto& bt1 = comp_config.boilingTemperature(1);
        const std::vector<double> ref_bt1 = {usys.to_si(M::temperature_absolute, 351), usys.to_si(M::temperature_absolute, 451), usys.to_si(M::temperature_absolute, 551)};
        check_vectors_close(ref_bt1, bt1, tolerance);
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
        const auto& zc0 = comp_config.criticalZFactor(0);
        BOOST_CHECK_EQUAL(num_comps, zc0.size());
        check_vectors_close(std::vector<double>{0.28, 0.25, 0.26}, zc0, tolerance);
        const auto& zc1 = comp_config.criticalZFactor(1);
        BOOST_CHECK_EQUAL(num_comps, zc1.size());
        check_vectors_close(std::vector<double>{0.29, 0.26, 0.27}, zc1, tolerance);
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

    {
        const auto& vs0 = comp_config.volumeShifts(0);
        BOOST_CHECK_EQUAL(num_comps, vs0.size());
        check_vectors_close(std::vector<double>{0.1, 0.2, 0.3}, vs0, tolerance);
        const auto& vs1 = comp_config.volumeShifts(1);
        BOOST_CHECK_EQUAL(num_comps, vs1.size());
        check_vectors_close(std::vector<double>{0.11, 0.21, 0.31}, vs1, tolerance);
    }

    // Surface-condition EOS region properties
    BOOST_CHECK(CompositionalConfig::EOSType::PRCORR == comp_config.eosTypeSurf(0));
    BOOST_CHECK(CompositionalConfig::EOSType::RK     == comp_config.eosTypeSurf(1));
    BOOST_CHECK(CompositionalConfig::EOSType::ZJ     == comp_config.eosTypeSurf(2));

    {
        const auto& acfs0 = comp_config.acentricFactorsSurf(0);
        const auto& acfs1 = comp_config.acentricFactorsSurf(1);
        const auto& acfs2 = comp_config.acentricFactorsSurf(2);
        BOOST_CHECK_EQUAL(num_comps, acfs0.size());
        BOOST_CHECK_EQUAL(num_comps, acfs1.size());
        BOOST_CHECK_EQUAL(num_comps, acfs2.size());
        check_vectors_close(std::vector<double>{0.41, 0.21, 0.011}, acfs0, tolerance);
        check_vectors_close(std::vector<double>{0.51, 0.31, 0.031}, acfs1, tolerance);
        check_vectors_close(std::vector<double>{0.52, 0.32, 0.032}, acfs2, tolerance);
    }

    {
        const std::vector<double> ref_cps0 = {usys.to_si(M::pressure, 22), usys.to_si(M::pressure, 72),
                                              usys.to_si(M::pressure, 42)};
        const std::vector<double> ref_cps1 = {usys.to_si(M::pressure, 23), usys.to_si(M::pressure, 73),
                                              usys.to_si(M::pressure, 43)};
        const std::vector<double> ref_cps2 = {usys.to_si(M::pressure, 24), usys.to_si(M::pressure, 74),
                                              usys.to_si(M::pressure, 44)};
        check_vectors_close(ref_cps0, comp_config.criticalPressureSurf(0), tolerance);
        check_vectors_close(ref_cps1, comp_config.criticalPressureSurf(1), tolerance);
        check_vectors_close(ref_cps2, comp_config.criticalPressureSurf(2), tolerance);
    }

    {
        const std::vector<double> ref_cts0 = {usys.to_si(M::temperature_absolute, 602),
                                              usys.to_si(M::temperature_absolute, 302),
                                              usys.to_si(M::temperature_absolute, 192)};
        const std::vector<double> ref_cts1 = {usys.to_si(M::temperature_absolute, 603),
                                              usys.to_si(M::temperature_absolute, 303),
                                              usys.to_si(M::temperature_absolute, 193)};
        const std::vector<double> ref_cts2 = {usys.to_si(M::temperature_absolute, 604),
                                              usys.to_si(M::temperature_absolute, 304),
                                              usys.to_si(M::temperature_absolute, 194)};
        check_vectors_close(ref_cts0, comp_config.criticalTemperatureSurf(0), tolerance);
        check_vectors_close(ref_cts1, comp_config.criticalTemperatureSurf(1), tolerance);
        check_vectors_close(ref_cts2, comp_config.criticalTemperatureSurf(2), tolerance);
    }

    {
        const std::vector<double> ref_bts0 = {usys.to_si(M::temperature_absolute, 352),
                                              usys.to_si(M::temperature_absolute, 452),
                                              usys.to_si(M::temperature_absolute, 552)};
        const std::vector<double> ref_bts1 = {usys.to_si(M::temperature_absolute, 353),
                                              usys.to_si(M::temperature_absolute, 453),
                                              usys.to_si(M::temperature_absolute, 553)};
        const std::vector<double> ref_bts2 = {usys.to_si(M::temperature_absolute, 354),
                                              usys.to_si(M::temperature_absolute, 454),
                                              usys.to_si(M::temperature_absolute, 554)};
        check_vectors_close(ref_bts0, comp_config.boilingTemperatureSurf(0), tolerance);
        check_vectors_close(ref_bts1, comp_config.boilingTemperatureSurf(1), tolerance);
        check_vectors_close(ref_bts2, comp_config.boilingTemperatureSurf(2), tolerance);
    }

    {
        const std::vector<double> ref_cvs0{usys.to_si("GeometricVolume/Moles", 0.62),
                                           usys.to_si("GeometricVolume/Moles", 0.12),
                                           usys.to_si("GeometricVolume/Moles", 0.12)};
        const std::vector<double> ref_cvs1{usys.to_si("GeometricVolume/Moles", 0.63),
                                           usys.to_si("GeometricVolume/Moles", 0.13),
                                           usys.to_si("GeometricVolume/Moles", 0.13)};
        const std::vector<double> ref_cvs2{usys.to_si("GeometricVolume/Moles", 0.64),
                                           usys.to_si("GeometricVolume/Moles", 0.14),
                                           usys.to_si("GeometricVolume/Moles", 0.14)};
        check_vectors_close(ref_cvs0, comp_config.criticalVolumeSurf(0), tolerance);
        check_vectors_close(ref_cvs1, comp_config.criticalVolumeSurf(1), tolerance);
        check_vectors_close(ref_cvs2, comp_config.criticalVolumeSurf(2), tolerance);
    }

    {
        check_vectors_close(std::vector<double>{0.30, 0.27, 0.28}, comp_config.criticalZFactorSurf(0), tolerance);
        check_vectors_close(std::vector<double>{0.31, 0.28, 0.29}, comp_config.criticalZFactorSurf(1), tolerance);
        check_vectors_close(std::vector<double>{0.32, 0.29, 0.30}, comp_config.criticalZFactorSurf(2), tolerance);
    }

    {
        constexpr std::size_t bic_size = num_comps * (num_comps - 1) / 2;
        const auto& bics0 = comp_config.binaryInteractionCoefficientSurf(0);
        const auto& bics1 = comp_config.binaryInteractionCoefficientSurf(1);
        const auto& bics2 = comp_config.binaryInteractionCoefficientSurf(2);
        BOOST_CHECK_EQUAL(bic_size, bics0.size());
        BOOST_CHECK_EQUAL(bic_size, bics1.size());
        BOOST_CHECK_EQUAL(bic_size, bics2.size());
        check_vectors_close(std::vector<double>{0, 0.1, 0.2}, bics0, tolerance);
        check_vectors_close(std::vector<double>{0.05, 0.15, 0.25}, bics1, tolerance);
        check_vectors_close(std::vector<double>{0.06, 0.16, 0.26}, bics2, tolerance);
    }

    {
        const std::vector<double> ref_mws0{usys.to_si("Mass/Moles", 143.),
                                           usys.to_si("Mass/Moles", 45.),
                                           usys.to_si("Mass/Moles", 17.)};
        const std::vector<double> ref_mws1{usys.to_si("Mass/Moles", 143.1),
                                           usys.to_si("Mass/Moles", 45.1),
                                           usys.to_si("Mass/Moles", 17.1)};
        const std::vector<double> ref_mws2{usys.to_si("Mass/Moles", 143.2),
                                           usys.to_si("Mass/Moles", 45.2),
                                           usys.to_si("Mass/Moles", 17.2)};
        check_vectors_close(ref_mws0, comp_config.molecularWeightsSurf(0), tolerance);
        check_vectors_close(ref_mws1, comp_config.molecularWeightsSurf(1), tolerance);
        check_vectors_close(ref_mws2, comp_config.molecularWeightsSurf(2), tolerance);
    }

    {
        check_vectors_close(std::vector<double>{0.12, 0.22, 0.32}, comp_config.volumeShiftsSurf(0), tolerance);
        check_vectors_close(std::vector<double>{0.13, 0.23, 0.33}, comp_config.volumeShiftsSurf(1), tolerance);
        check_vectors_close(std::vector<double>{0.14, 0.24, 0.34}, comp_config.volumeShiftsSurf(2), tolerance);
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

SSHIFT
0.1 0.2 0.3 /
0.11 0.21 0.31 /


ZCRIT
0.28 0.25 0.26 /
0.29 0.26 0.27 /

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

BOOST_AUTO_TEST_CASE(PRCORRKeywordTest) {
    // Minimal compositional deck used to verify the handling of the PRCORR
    // keyword. There are two EOS regions: the first uses PR (and must be
    // promoted to PRCORR by PRCORR), the second uses SRK (and must be left
    // unchanged by PRCORR).
    const std::string deck_string = R"(
RUNSPEC

METRIC

TABDIMS
 8* 2 3 /

OIL
GAS

DIMENS
 1 1 1 /

COMPS
 2 /

GRID

DX
 1*10 /
DY
 1*10 /
DZ
 1*10 /
TOPS
 1*0 /
PERMX
 1*100 /
PERMY
 1*100 /
PERMZ
 1*100 /
PORO
 1*0.1 /

PROPS

CNAMES
 C1
 C10
/

EOS
 PR /
 SRK /

PRCORR

END
)";

    const Deck deck = Parser{}.parseString(deck_string);
    const Runspec runspec{deck};
    const CompositionalConfig comp_config{deck, runspec};

    BOOST_CHECK(CompositionalConfig::EOSType::PRCORR == comp_config.eosType(0));
    // PRCORR should have no effect on the second region since it is SRK.
    BOOST_CHECK(CompositionalConfig::EOSType::SRK == comp_config.eosType(1));

    // PRCORR also affects surface EOS regions inherited from reservoir EOS.
    BOOST_CHECK(CompositionalConfig::EOSType::PRCORR == comp_config.eosTypeSurf(0));
    BOOST_CHECK(CompositionalConfig::EOSType::SRK    == comp_config.eosTypeSurf(1));
    BOOST_CHECK(CompositionalConfig::EOSType::SRK    == comp_config.eosTypeSurf(2));
}

Deck createCompositionalDeckSurfaceDefaults()
{
    // NMEOSR=2, NMEOSS=3, no surface keywords: surface values must inherit from reservoir values.
    return Parser{}.parseString(R"(
RUNSPEC
TITLE
   SURFACE DEFAULTS TEST

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

GRID
DX
4*10 /
DY
4*1 /
DZ
4*1 /
TOPS
4*0 /
PERMX
4*100 /
PERMY
4*100 /
PERMZ
4*100 /
PORO
4*0.1 /

PROPS

CNAMES
DECANE
CO2
METHANE
/

EOS
PR /
SRK /

MW
142.  44.  16. /
142.1 44.1 16.1 /

ACF
0.4 0.2 0.01 /
0.5 0.3 0.03 /

PCRIT
20. 70. 40. /
21. 71. 41. /

TCRIT
600. 300. 190. /
601. 301. 191. /

TBOIL
350. 450. 550. /
351. 451. 551. /

VCRIT
0.6  0.1  0.1 /
0.61 0.11 0.11 /

ZCRIT
0.28 0.25 0.26 /
0.29 0.26 0.27 /

SSHIFT
0.1 0.2 0.3 /
0.11 0.21 0.31 /

BIC
0
1 2 /
1
2 3 /

END
)");
}

BOOST_AUTO_TEST_CASE(SurfaceDefaultsFromReservoirTest) {
    const Deck deck = createCompositionalDeckSurfaceDefaults();
    const Runspec runspec{deck};
    const CompositionalConfig comp_config{deck, runspec};

    constexpr std::size_t num_comps = 3;
    constexpr double tolerance = 1.e-5;
    const auto& usys = deck.getActiveUnitSystem();
    using M = UnitSystem::measure;

    // Without EOSS, surface EOS types inherit from reservoir EOS (last region repeated).
    BOOST_CHECK(CompositionalConfig::EOSType::PR  == comp_config.eosTypeSurf(0));
    BOOST_CHECK(CompositionalConfig::EOSType::SRK == comp_config.eosTypeSurf(1));
    BOOST_CHECK(CompositionalConfig::EOSType::SRK == comp_config.eosTypeSurf(2));

    // Without MWS, molecular weights inherit from MW.
    {
        const auto& mws0 = comp_config.molecularWeightsSurf(0);
        const auto& mws1 = comp_config.molecularWeightsSurf(1);
        const auto& mws2 = comp_config.molecularWeightsSurf(2);
        BOOST_CHECK_EQUAL(num_comps, mws0.size());
        check_vectors_close(comp_config.molecularWeights(0), mws0, tolerance);
        check_vectors_close(comp_config.molecularWeights(1), mws1, tolerance);
        check_vectors_close(comp_config.molecularWeights(1), mws2, tolerance);
    }

    // Without ACFS, acentric factors inherit from ACF.
    check_vectors_close(std::vector<double>{0.4, 0.2, 0.01},
                        comp_config.acentricFactorsSurf(0), tolerance);
    check_vectors_close(std::vector<double>{0.5, 0.3, 0.03},
                        comp_config.acentricFactorsSurf(1), tolerance);
    check_vectors_close(std::vector<double>{0.5, 0.3, 0.03},
                        comp_config.acentricFactorsSurf(2), tolerance);

    // Without PCRITS, critical pressure inherits from PCRIT.
    {
        const std::vector<double> ref_pcs0{usys.to_si(M::pressure, 20),
                                           usys.to_si(M::pressure, 70),
                                           usys.to_si(M::pressure, 40)};
        const std::vector<double> ref_pcs1{usys.to_si(M::pressure, 21),
                                           usys.to_si(M::pressure, 71),
                                           usys.to_si(M::pressure, 41)};
        check_vectors_close(ref_pcs0, comp_config.criticalPressureSurf(0), tolerance);
        check_vectors_close(ref_pcs1, comp_config.criticalPressureSurf(1), tolerance);
        check_vectors_close(ref_pcs1, comp_config.criticalPressureSurf(2), tolerance);
    }

    // Without TCRITS, critical temperature inherits from TCRIT.
    {
        const std::vector<double> ref_tcs0{usys.to_si(M::temperature_absolute, 600),
                                           usys.to_si(M::temperature_absolute, 300),
                                           usys.to_si(M::temperature_absolute, 190)};
        const std::vector<double> ref_tcs1{usys.to_si(M::temperature_absolute, 601),
                                           usys.to_si(M::temperature_absolute, 301),
                                           usys.to_si(M::temperature_absolute, 191)};
        check_vectors_close(ref_tcs0, comp_config.criticalTemperatureSurf(0), tolerance);
        check_vectors_close(ref_tcs1, comp_config.criticalTemperatureSurf(1), tolerance);
        check_vectors_close(ref_tcs1, comp_config.criticalTemperatureSurf(2), tolerance);
    }

    // Without TBOILS, boiling temperature inherits from TBOIL.
    {
        const std::vector<double> ref_tbs0{usys.to_si(M::temperature_absolute, 350),
                                           usys.to_si(M::temperature_absolute, 450),
                                           usys.to_si(M::temperature_absolute, 550)};
        const std::vector<double> ref_tbs1{usys.to_si(M::temperature_absolute, 351),
                                           usys.to_si(M::temperature_absolute, 451),
                                           usys.to_si(M::temperature_absolute, 551)};
        check_vectors_close(ref_tbs0, comp_config.boilingTemperatureSurf(0), tolerance);
        check_vectors_close(ref_tbs1, comp_config.boilingTemperatureSurf(1), tolerance);
        check_vectors_close(ref_tbs1, comp_config.boilingTemperatureSurf(2), tolerance);
    }

    // Without VCRITS, critical volume inherits from VCRIT.
    {
        const std::vector<double> ref_vcs0{usys.to_si("GeometricVolume/Moles", 0.6),
                                           usys.to_si("GeometricVolume/Moles", 0.1),
                                           usys.to_si("GeometricVolume/Moles", 0.1)};
        const std::vector<double> ref_vcs1{usys.to_si("GeometricVolume/Moles", 0.61),
                                           usys.to_si("GeometricVolume/Moles", 0.11),
                                           usys.to_si("GeometricVolume/Moles", 0.11)};
        check_vectors_close(ref_vcs0, comp_config.criticalVolumeSurf(0), tolerance);
        check_vectors_close(ref_vcs1, comp_config.criticalVolumeSurf(1), tolerance);
        check_vectors_close(ref_vcs1, comp_config.criticalVolumeSurf(2), tolerance);
    }

    // Without ZCRITS, critical Z-factor inherits from ZCRIT.
    check_vectors_close(std::vector<double>{0.28, 0.25, 0.26},
                        comp_config.criticalZFactorSurf(0), tolerance);
    check_vectors_close(std::vector<double>{0.29, 0.26, 0.27},
                        comp_config.criticalZFactorSurf(1), tolerance);
    check_vectors_close(std::vector<double>{0.29, 0.26, 0.27},
                        comp_config.criticalZFactorSurf(2), tolerance);

    // Without SSHIFTS, volume shifts inherit from SSHIFT.
    check_vectors_close(std::vector<double>{0.1, 0.2, 0.3},
                        comp_config.volumeShiftsSurf(0), tolerance);
    check_vectors_close(std::vector<double>{0.11, 0.21, 0.31},
                        comp_config.volumeShiftsSurf(1), tolerance);
    check_vectors_close(std::vector<double>{0.11, 0.21, 0.31},
                        comp_config.volumeShiftsSurf(2), tolerance);

    // Without BICS, BIC values inherit from BIC.
    {
        constexpr std::size_t bic_size = num_comps * (num_comps - 1) / 2;
        BOOST_CHECK_EQUAL(bic_size, comp_config.binaryInteractionCoefficientSurf(0).size());
        BOOST_CHECK_EQUAL(bic_size, comp_config.binaryInteractionCoefficientSurf(1).size());
        BOOST_CHECK_EQUAL(bic_size, comp_config.binaryInteractionCoefficientSurf(2).size());
        check_vectors_close(std::vector<double>{0, 1, 2},
                            comp_config.binaryInteractionCoefficientSurf(0), tolerance);
        check_vectors_close(std::vector<double>{1, 2, 3},
                            comp_config.binaryInteractionCoefficientSurf(1), tolerance);
        // Last surface region repeats the last reservoir region.
        check_vectors_close(std::vector<double>{1, 2, 3},
                            comp_config.binaryInteractionCoefficientSurf(2), tolerance);
    }
}

BOOST_AUTO_TEST_CASE(SurfacePartialSpecificationTest) {
    // Partial surface keyword input: specified keywords use keyword defaults, not reservoir fallback.
    const std::string deck_string = R"(
RUNSPEC
TITLE
   SURFACE PARTIAL TEST

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

GRID
DX
4*10 /
DY
4*1 /
DZ
4*1 /
TOPS
4*0 /
PERMX
4*100 /
PERMY
4*100 /
PERMZ
4*100 /
PORO
4*0.1 /

PROPS

CNAMES
DECANE
CO2
METHANE
/

EOS
PR /
SRK /

MW
142.  44.  16. /
142.1 44.1 16.1 /

ACF
0.4 0.2 0.01 /
0.5 0.3 0.03 /

PCRIT
20. 70. 40. /
21. 71. 41. /

TCRIT
600. 300. 190. /
601. 301. 191. /

TBOIL
350. 450. 550. /
351. 451. 551. /

VCRIT
0.6  0.1  0.1 /
0.61 0.11 0.11 /

ZCRIT
0.28 0.25 0.26 /
0.29 0.26 0.27 /

SSHIFT
0.1 0.2 0.3 /
0.11 0.21 0.31 /

BIC
0
1 2 /
1
2 3 /

-- Partial EOSS: unspecified records use keyword default PR, not reservoir EOS.
EOSS
ZJ /
/
/

-- Partial SSHIFTS: unspecified values use keyword default 0.0.
SSHIFTS
0.5 /
/
/

-- Partial BICS: unspecified values use keyword default 0.0.
BICS
0.7 /
/
/

END
)";

    const Deck deck = Parser{}.parseString(deck_string);
    const Runspec runspec{deck};
    const CompositionalConfig comp_config{deck, runspec};

    constexpr std::size_t num_comps = 3;
    constexpr double tolerance = 1.e-5;
    const auto& usys = deck.getActiveUnitSystem();
    using M = UnitSystem::measure;

    // Partial EOSS: unspecified records use EOSS default (PR), not reservoir EOS.
    BOOST_CHECK(CompositionalConfig::EOSType::ZJ == comp_config.eosTypeSurf(0));
    BOOST_CHECK(CompositionalConfig::EOSType::PR == comp_config.eosTypeSurf(1));
    BOOST_CHECK(CompositionalConfig::EOSType::PR == comp_config.eosTypeSurf(2));

    // Partial SSHIFTS: missing entries default to 0.0.
    {
        const auto& vss0 = comp_config.volumeShiftsSurf(0);
        BOOST_CHECK_EQUAL(num_comps, vss0.size());
        check_vectors_close(std::vector<double>{0.5, 0.0, 0.0}, vss0, tolerance);
        check_vectors_close(std::vector<double>{0.0, 0.0, 0.0},
                            comp_config.volumeShiftsSurf(1), tolerance);
        check_vectors_close(std::vector<double>{0.0, 0.0, 0.0},
                            comp_config.volumeShiftsSurf(2), tolerance);
    }

    // Partial BICS: same defaulting behavior.
    {
        constexpr std::size_t bic_size = num_comps * (num_comps - 1) / 2;
        const auto& bics0 = comp_config.binaryInteractionCoefficientSurf(0);
        BOOST_CHECK_EQUAL(bic_size, bics0.size());
        check_vectors_close(std::vector<double>{0.7, 0.0, 0.0}, bics0, tolerance);
        check_vectors_close(std::vector<double>{0.0, 0.0, 0.0},
                            comp_config.binaryInteractionCoefficientSurf(1), tolerance);
        check_vectors_close(std::vector<double>{0.0, 0.0, 0.0},
                            comp_config.binaryInteractionCoefficientSurf(2), tolerance);
    }

    // Keywords not provided at all still inherit from reservoir values.
    check_vectors_close(std::vector<double>{0.4, 0.2, 0.01},
                        comp_config.acentricFactorsSurf(0), tolerance);
    check_vectors_close(std::vector<double>{0.5, 0.3, 0.03},
                        comp_config.acentricFactorsSurf(1), tolerance);
    check_vectors_close(std::vector<double>{0.5, 0.3, 0.03},
                        comp_config.acentricFactorsSurf(2), tolerance);

    check_vectors_close(std::vector<double>{usys.to_si(M::pressure, 20),
                                            usys.to_si(M::pressure, 70),
                                            usys.to_si(M::pressure, 40)},
                        comp_config.criticalPressureSurf(0), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si(M::pressure, 21),
                                            usys.to_si(M::pressure, 71),
                                            usys.to_si(M::pressure, 41)},
                        comp_config.criticalPressureSurf(1), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si(M::pressure, 21),
                                            usys.to_si(M::pressure, 71),
                                            usys.to_si(M::pressure, 41)},
                        comp_config.criticalPressureSurf(2), tolerance);

    check_vectors_close(std::vector<double>{usys.to_si(M::temperature_absolute, 600),
                                            usys.to_si(M::temperature_absolute, 300),
                                            usys.to_si(M::temperature_absolute, 190)},
                        comp_config.criticalTemperatureSurf(0), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si(M::temperature_absolute, 601),
                                            usys.to_si(M::temperature_absolute, 301),
                                            usys.to_si(M::temperature_absolute, 191)},
                        comp_config.criticalTemperatureSurf(1), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si(M::temperature_absolute, 601),
                                            usys.to_si(M::temperature_absolute, 301),
                                            usys.to_si(M::temperature_absolute, 191)},
                        comp_config.criticalTemperatureSurf(2), tolerance);

    check_vectors_close(std::vector<double>{usys.to_si(M::temperature_absolute, 350),
                                            usys.to_si(M::temperature_absolute, 450),
                                            usys.to_si(M::temperature_absolute, 550)},
                        comp_config.boilingTemperatureSurf(0), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si(M::temperature_absolute, 351),
                                            usys.to_si(M::temperature_absolute, 451),
                                            usys.to_si(M::temperature_absolute, 551)},
                        comp_config.boilingTemperatureSurf(1), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si(M::temperature_absolute, 351),
                                            usys.to_si(M::temperature_absolute, 451),
                                            usys.to_si(M::temperature_absolute, 551)},
                        comp_config.boilingTemperatureSurf(2), tolerance);

    check_vectors_close(std::vector<double>{usys.to_si("GeometricVolume/Moles", 0.6),
                                            usys.to_si("GeometricVolume/Moles", 0.1),
                                            usys.to_si("GeometricVolume/Moles", 0.1)},
                        comp_config.criticalVolumeSurf(0), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si("GeometricVolume/Moles", 0.61),
                                            usys.to_si("GeometricVolume/Moles", 0.11),
                                            usys.to_si("GeometricVolume/Moles", 0.11)},
                        comp_config.criticalVolumeSurf(1), tolerance);
    check_vectors_close(std::vector<double>{usys.to_si("GeometricVolume/Moles", 0.61),
                                            usys.to_si("GeometricVolume/Moles", 0.11),
                                            usys.to_si("GeometricVolume/Moles", 0.11)},
                        comp_config.criticalVolumeSurf(2), tolerance);

    check_vectors_close(std::vector<double>{0.28, 0.25, 0.26},
                        comp_config.criticalZFactorSurf(0), tolerance);
    check_vectors_close(std::vector<double>{0.29, 0.26, 0.27},
                        comp_config.criticalZFactorSurf(1), tolerance);
    check_vectors_close(std::vector<double>{0.29, 0.26, 0.27},
                        comp_config.criticalZFactorSurf(2), tolerance);

    check_vectors_close(comp_config.molecularWeights(0),
                        comp_config.molecularWeightsSurf(0), tolerance);
    check_vectors_close(comp_config.molecularWeights(1),
                        comp_config.molecularWeightsSurf(1), tolerance);
    check_vectors_close(comp_config.molecularWeights(1),
                        comp_config.molecularWeightsSurf(2), tolerance);
}

BOOST_AUTO_TEST_CASE(SurfaceKeywordRequiresReservoirKeywordTest) {
    const std::string deck_string = R"(
RUNSPEC
TITLE
   SURFACE REQUIRES RESERVOIR TEST

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

GRID
DX
4*10 /
DY
4*1 /
DZ
4*1 /
TOPS
4*0 /
PERMX
4*100 /
PERMY
4*100 /
PERMZ
4*100 /
PORO
4*0.1 /

PROPS

CNAMES
DECANE
CO2
METHANE
/

EOS
PR /
SRK /

MWS
143. 45. 17. /
143.1 45.1 17.1 /
143.2 45.2 17.2 /

END
)";

    const Deck deck = Parser{}.parseString(deck_string);
    const Runspec runspec{deck};

    const auto construct = [&]() { CompositionalConfig config{deck, runspec}; };
    BOOST_CHECK_THROW(construct(), OpmInputError);
}

}
