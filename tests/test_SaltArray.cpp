/*
  Copyright 2026 NORCE.

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
#include <config.h>

#include <opm/common/utility/SaltArray.hpp>
#include <opm/material/densead/Evaluation.hpp>

#define BOOST_TEST_MODULE SaltArrayTests
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

using namespace Opm;

using EvalTypes = boost::mpl::list<float,
                                   double,
                                   DenseAd::Evaluation<float, 3>,
                                   DenseAd::Evaluation<double, 3> >;

BOOST_AUTO_TEST_CASE_TEMPLATE(ConstructorTests, Eval, EvalTypes)
{
    // Default constructor
    SaltArray<Eval, SaltMassFraction> saltMassFrac;
    BOOST_CHECK(!saltMassFrac.any_nonzero());
    SaltArray<Eval, SaltMoleFraction> saltMoleFrac;
    BOOST_CHECK(!saltMoleFrac.any_nonzero());
    SaltArray<Eval, SaltMolality> saltMolal;
    BOOST_CHECK(!saltMolal.any_nonzero());

    // Copy constructor
    for (std::size_t i = 0; i < saltMassFrac.size(); ++i) {
        auto index = static_cast<SaltIndex>(i);
        saltMassFrac[index] = static_cast<Eval>(i + 1);
        saltMoleFrac[index] = static_cast<Eval>(i + 1);
        saltMolal[index] = static_cast<Eval>(i + 1);
    }
    SaltArray<Eval, SaltMassFraction> saltMassFrac2(saltMassFrac);
    BOOST_CHECK_EQUAL_COLLECTIONS(saltMassFrac.begin(),
                                  saltMassFrac.end(),
                                  saltMassFrac2.begin(),
                                  saltMassFrac2.end());
    SaltArray<Eval, SaltMoleFraction> saltMoleFrac2(saltMoleFrac);
    BOOST_CHECK_EQUAL_COLLECTIONS(saltMoleFrac.begin(),
                                  saltMoleFrac.end(),
                                  saltMoleFrac2.begin(),
                                  saltMoleFrac2.end());
    SaltArray<Eval, SaltMolality> saltMolal2(saltMolal);
    BOOST_CHECK_EQUAL_COLLECTIONS(saltMolal.begin(),
                                  saltMolal.end(),
                                  saltMolal2.begin(),
                                  saltMolal2.end());

    // Conversion constructor (only double/float to Evaluation<double>/Evaluation<float>)
    if constexpr (std::is_same_v<Eval, float> || std::is_same_v<Eval, double>) {
        SaltArray<DenseAd::Evaluation<Eval, 3>, SaltMassFraction> saltMassFracEval(saltMassFrac);
        BOOST_CHECK(saltMassFracEval.any_nonzero());
        BOOST_CHECK_EQUAL_COLLECTIONS(saltMassFracEval.begin(),
                                      saltMassFracEval.end(),
                                      saltMassFrac.begin(),
                                      saltMassFrac.end());
        SaltArray<DenseAd::Evaluation<Eval, 3>, SaltMoleFraction> saltMoleFracEval(saltMoleFrac);
        BOOST_CHECK(saltMoleFracEval.any_nonzero());
        BOOST_CHECK_EQUAL_COLLECTIONS(saltMoleFracEval.begin(),
                                      saltMoleFracEval.end(),
                                      saltMoleFrac.begin(),
                                      saltMoleFrac.end());
        SaltArray<DenseAd::Evaluation<Eval, 3>, SaltMolality> saltMolalEval(saltMolal);
        BOOST_CHECK(saltMolalEval.any_nonzero());
        BOOST_CHECK_EQUAL_COLLECTIONS(saltMolalEval.begin(),
                                      saltMolalEval.end(),
                                      saltMolal.begin(),
                                      saltMolal.end());
    }

    // Test clear (i.e., fill array with zeros)
    saltMolal.clear();
    saltMassFrac.clear();
    saltMoleFrac.clear();
    BOOST_CHECK(!saltMolal.any_nonzero());
    BOOST_CHECK(!saltMassFrac.any_nonzero());
    BOOST_CHECK(!saltMoleFrac.any_nonzero());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(CopyAssignmentTests, Eval, EvalTypes)
{
    // Setup
    SaltArray<Eval, SaltMassFraction> saltMassFrac;
    SaltArray<Eval, SaltMoleFraction> saltMoleFrac;
    SaltArray<Eval, SaltMolality> saltMolal;
    for (std::size_t i = 0; i < saltMassFrac.size(); ++i) {
        auto index = static_cast<SaltIndex>(i);
        saltMassFrac[index] = static_cast<Eval>(i + 1);
        saltMoleFrac[index] = static_cast<Eval>(i + 1);
        saltMolal[index] = static_cast<Eval>(i + 1);
    }

    // Copy assignment, same type
    SaltArray<Eval, SaltMassFraction> saltMassFrac2 = saltMassFrac;
    SaltArray<Eval, SaltMoleFraction> saltMoleFrac2 = saltMoleFrac;
    SaltArray<Eval, SaltMolality> saltMolal2 = saltMolal;
    BOOST_CHECK_EQUAL_COLLECTIONS(saltMassFrac.begin(),
                                  saltMassFrac.end(),
                                  saltMassFrac2.begin(),
                                  saltMassFrac2.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(saltMoleFrac.begin(),
                                  saltMoleFrac.end(),
                                  saltMoleFrac2.begin(),
                                  saltMoleFrac2.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(saltMolal.begin(),
                                  saltMolal.end(),
                                  saltMolal2.begin(),
                                  saltMolal2.end());

    // Copy assignment, conversion (only double/float to Evaluation<double>/Evaluation<float>)
    if constexpr (std::is_same_v<Eval, float> || std::is_same_v<Eval, double>) {
        SaltArray<DenseAd::Evaluation<Eval, 3>, SaltMassFraction> saltMassFracEval;
        saltMassFracEval = saltMassFrac;
        BOOST_CHECK(saltMassFracEval.any_nonzero());
        BOOST_CHECK_EQUAL_COLLECTIONS(saltMassFracEval.begin(),
                                      saltMassFracEval.end(),
                                      saltMassFrac.begin(),
                                      saltMassFrac.end());
        SaltArray<DenseAd::Evaluation<Eval, 3>, SaltMoleFraction> saltMoleFracEval;
        saltMoleFracEval = saltMoleFrac;
        BOOST_CHECK(saltMoleFracEval.any_nonzero());
        BOOST_CHECK_EQUAL_COLLECTIONS(saltMoleFracEval.begin(),
                                      saltMoleFracEval.end(),
                                      saltMoleFrac.begin(),
                                      saltMoleFrac.end());
        SaltArray<DenseAd::Evaluation<Eval, 3>, SaltMolality> saltMolalEval;
        saltMolalEval = saltMolal;
        BOOST_CHECK(saltMolalEval.any_nonzero());
        BOOST_CHECK_EQUAL_COLLECTIONS(saltMolalEval.begin(),
                                      saltMolalEval.end(),
                                      saltMolal.begin(),
                                      saltMolal.end());
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnitConversionTests, Eval, EvalTypes)
{
    // Setup
    SaltArray<Eval, SaltMolality> expectedSaltMolal;
    expectedSaltMolal[SaltIndex::NA] = 1.0;
    expectedSaltMolal[SaltIndex::K] = 2.0;
    expectedSaltMolal[SaltIndex::CA] = 3.0;
    expectedSaltMolal[SaltIndex::MG] = 4.0;
    expectedSaltMolal[SaltIndex::CL] = 5.0;
    expectedSaltMolal[SaltIndex::SO4] = 6.0;

    SaltArray<Eval, SaltMoleFraction> expectedSaltMoleFrac;
    expectedSaltMoleFrac[SaltIndex::NA] = 0.0130712;
    expectedSaltMoleFrac[SaltIndex::K] = 0.0261424;
    expectedSaltMoleFrac[SaltIndex::CA] = 0.0392136;
    expectedSaltMoleFrac[SaltIndex::MG] = 0.0522848;
    expectedSaltMoleFrac[SaltIndex::CL] = 0.0653560;
    expectedSaltMoleFrac[SaltIndex::SO4] = 0.0784272;
    Eval expectedH2OMoleFrac = 0.725998;

    SaltArray<Eval, SaltMassFraction> expectedSaltMassFrac;
    expectedSaltMassFrac[SaltIndex::NA] = 0.0110850;
    expectedSaltMassFrac[SaltIndex::K] = 0.0377240;
    expectedSaltMassFrac[SaltIndex::CA] = 0.0580360;
    expectedSaltMassFrac[SaltIndex::MG] = 0.0469030;
    expectedSaltMassFrac[SaltIndex::CL] = 0.0855510;
    expectedSaltMassFrac[SaltIndex::SO4] = 0.278418;
    Eval expectedH2OMassFrac = 0.482283;

    //
    // Conversion from molality
    //
    SaltArray<Eval, SaltMassFraction> convSaltMassFrac =
        expectedSaltMolal.template convert_to<SaltMassFraction>();
    SaltArray<Eval, SaltMoleFraction> convSaltMoleFrac
        = expectedSaltMolal.template convert_to<SaltMoleFraction>();
    for (std::size_t i = 0; i < expectedSaltMolal.size(); ++i) {
        auto index = static_cast<SaltIndex>(i);
        BOOST_CHECK_CLOSE(convSaltMoleFrac[index], expectedSaltMoleFrac[index], 5.0e-1);
        BOOST_CHECK_CLOSE(convSaltMassFrac[index], expectedSaltMassFrac[index], 5.0e-1);
    }
    BOOST_CHECK_CLOSE(1.0 - convSaltMassFrac.sum(), expectedH2OMassFrac, 5.0e-1);
    BOOST_CHECK_CLOSE(1.0 - convSaltMoleFrac.sum(), expectedH2OMoleFrac, 5.0e-1);

    //
    // Conversion from mass fraction
    //
    SaltArray<Eval, SaltMolality> convSaltMolal =
        expectedSaltMassFrac.template convert_to<SaltMolality>();
    SaltArray<Eval, SaltMoleFraction> convSaltMoleFrac2 =
        expectedSaltMassFrac.template convert_to<SaltMoleFraction>();
    for (std::size_t i = 0; i < expectedSaltMassFrac.size(); ++i) {
        auto index = static_cast<SaltIndex>(i);
        BOOST_CHECK_CLOSE(convSaltMolal[index], expectedSaltMolal[index], 5.0e-1);
        BOOST_CHECK_CLOSE(convSaltMoleFrac2[index], expectedSaltMoleFrac[index], 5.0e-1);
    }
    BOOST_CHECK_CLOSE(1.0 - convSaltMoleFrac2.sum(), expectedH2OMoleFrac, 5.0e-1);

    //
    // Conversion from mole fraction
    //
    SaltArray<Eval, SaltMolality> convSaltMolal2 =
        expectedSaltMoleFrac.template convert_to<SaltMolality>();
    SaltArray<Eval, SaltMassFraction> convSaltMassFrac2 =
        expectedSaltMoleFrac.template convert_to<SaltMassFraction>();
    for (std::size_t i = 0; i < expectedSaltMassFrac.size(); ++i) {
        auto index = static_cast<SaltIndex>(i);
        BOOST_CHECK_CLOSE(convSaltMolal2[index], expectedSaltMolal[index], 5.0e-1);
        BOOST_CHECK_CLOSE(convSaltMassFrac2[index], expectedSaltMassFrac[index], 5.0e-1);
    }
    BOOST_CHECK_CLOSE(1.0 - convSaltMassFrac2.sum(), expectedH2OMassFrac, 5.0e-1);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(CationAnionTests, Eval, EvalTypes)
{
    // Both cations and anions
    SaltArray<Eval, SaltMolality> saltMolal;
    saltMolal[SaltIndex::MG] = 1.0;
    saltMolal[SaltIndex::SO4] = 1.0;
    saltMolal[SaltIndex::K] = 2.0;
    saltMolal[SaltIndex::CL] = 2.0;
    auto [cations, anions] = saltMolal.cations_and_anions();
    BOOST_CHECK(!cations.empty());
    BOOST_CHECK(!anions.empty());
    BOOST_CHECK(cations.size() == 2);
    BOOST_CHECK(anions.size() == 2);

    // Cations and anions in descending ion strength
    BOOST_CHECK(cations[0] == SaltIndex::MG);
    BOOST_CHECK(cations[1] == SaltIndex::K);
    BOOST_CHECK(anions[0] == SaltIndex::SO4);
    BOOST_CHECK(anions[1] == SaltIndex::CL);

    // Only cations
    SaltArray<Eval, SaltMolality> saltMolal2;
    saltMolal2[SaltIndex::NA] = 1.0;
    saltMolal2[SaltIndex::CA] = 2.0;
    auto [cations2, anions2] = saltMolal2.cations_and_anions();
    BOOST_CHECK(!cations2.empty());
    BOOST_CHECK(anions2.empty());
    BOOST_CHECK(cations2.size() == 2);

    // Cations in descending ion strength
    BOOST_CHECK(cations2[0] == SaltIndex::CA);
    BOOST_CHECK(cations2[1] == SaltIndex::NA);

    // Only anions
    SaltArray<Eval, SaltMolality> saltMolal3;
    saltMolal3[SaltIndex::CL] = 1.0;
    saltMolal3[SaltIndex::SO4] = 2.0;
    auto [cations3, anions3] = saltMolal3.cations_and_anions();
    BOOST_CHECK(cations3.empty());
    BOOST_CHECK(!anions3.empty());
    BOOST_CHECK(anions3.size() == 2);

    // Anions in descending ion strength
    BOOST_CHECK(anions3[0] == SaltIndex::SO4);
    BOOST_CHECK(anions3[1] == SaltIndex::CL);
}
