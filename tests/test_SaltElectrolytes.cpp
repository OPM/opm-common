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

#include <opm/common/utility/SaltElectrolytes.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/components/CaIon.hpp>
#include <opm/material/components/ClIon.hpp>
#include <opm/material/components/KIon.hpp>
#include <opm/material/components/MgIon.hpp>
#include <opm/material/components/NaIon.hpp>
#include <opm/material/components/SO4Ion.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>

#include <vector>

#define BOOST_TEST_MODULE SaltElectrolytesTests
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

using namespace Opm;

using EvalTypes = boost::mpl::list<float,
                                   double,
                                   DenseAd::Evaluation<float, 3>,
                                   DenseAd::Evaluation<double, 3> >;

BOOST_AUTO_TEST_CASE_TEMPLATE(MolarMassTests, Eval, EvalTypes)
{
    using Scalar = typename MathToolbox<Eval>::Scalar;
    using Electrolyte = typename SaltElectrolytes<Scalar>::Electrolyte;

    const Scalar tol = static_cast<Scalar>(1.0e-4);
    BOOST_CHECK_CLOSE(SaltElectrolytes<Scalar>::molarMass(Electrolyte::NaCl),
                       NaIon<Scalar>::molarMass() + ClIon<Scalar>::molarMass(),
                       tol);
    BOOST_CHECK_CLOSE(SaltElectrolytes<Scalar>::molarMass(Electrolyte::KCl),
                       KIon<Scalar>::molarMass() + ClIon<Scalar>::molarMass(),
                       tol);
    BOOST_CHECK_CLOSE(SaltElectrolytes<Scalar>::molarMass(Electrolyte::CaCl2),
                       CaIon<Scalar>::molarMass() + 2.0 * ClIon<Scalar>::molarMass(),
                       tol);
    BOOST_CHECK_CLOSE(SaltElectrolytes<Scalar>::molarMass(Electrolyte::MgCl2),
                       MgIon<Scalar>::molarMass() + 2.0 * ClIon<Scalar>::molarMass(),
                       tol);
    BOOST_CHECK_CLOSE(SaltElectrolytes<Scalar>::molarMass(Electrolyte::MgSO4),
                       MgIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass(),
                       tol);
    BOOST_CHECK_CLOSE(SaltElectrolytes<Scalar>::molarMass(Electrolyte::K2SO4),
                       2.0 * KIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass(),
                       tol);
    BOOST_CHECK_CLOSE(SaltElectrolytes<Scalar>::molarMass(Electrolyte::Na2SO4),
                       2.0 * NaIon<Scalar>::molarMass() + SO4Ion<Scalar>::molarMass(),
                       tol);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ZeroSalinityTests, Eval, EvalTypes)
{
    using Scalar = typename MathToolbox<Eval>::Scalar;

    // Default-constructed SaltArray is all zero: no electrolyte can be formed
    SaltArray<Eval, SaltMassFraction> salinity;
    auto electrolytes = SaltElectrolytes<Scalar>::fromSaltComponents(salinity);
    BOOST_CHECK(electrolytes.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SingleElectrolyteTests, Eval, EvalTypes)
{
    using Scalar = typename MathToolbox<Eval>::Scalar;
    using Electrolyte = typename SaltElectrolytes<Scalar>::Electrolyte;

    // Stoichiometrically balanced cation/anion molalities (no leftover after pairing) for
    // each supported electrolyte
    struct Case
    {
        SaltIndex cationIdx;
        Scalar cationMolal;
        SaltIndex anionIdx;
        Scalar anionMolal;
        Electrolyte electrolyte;
    };
    const std::vector<Case> cases = {
        {SaltIndex::NA, 0.7, SaltIndex::CL, 0.7, Electrolyte::NaCl},
        {SaltIndex::K,  0.7, SaltIndex::CL, 0.7, Electrolyte::KCl},
        {SaltIndex::CA, 0.7, SaltIndex::CL, 1.4, Electrolyte::CaCl2},
        {SaltIndex::MG, 0.7, SaltIndex::CL, 1.4, Electrolyte::MgCl2},
        {SaltIndex::MG, 0.7, SaltIndex::SO4, 0.7, Electrolyte::MgSO4},
        {SaltIndex::NA, 1.4, SaltIndex::SO4, 0.7, Electrolyte::Na2SO4},
        {SaltIndex::K,  1.4, SaltIndex::SO4, 0.7, Electrolyte::K2SO4},
    };

    for (const auto& c : cases) {
        SaltArray<Eval, SaltMolality> molal;
        molal[c.cationIdx] = c.cationMolal;
        molal[c.anionIdx] = c.anionMolal;
        SaltArray<Eval, SaltMassFraction> salinity = molal.template convert_to<SaltMassFraction>();

        auto electrolytes = SaltElectrolytes<Scalar>::fromSaltComponents(salinity);
        BOOST_REQUIRE_EQUAL(electrolytes.size(), 1U);
        BOOST_CHECK(electrolytes[0].first == c.electrolyte);

        // No residual molality: the single electrolyte's mass fraction reconstructs the
        // full dissolved-salt mass fraction
        BOOST_CHECK_CLOSE(electrolytes[0].second, salinity.sum(), static_cast<Scalar>(1.0e-4));
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(MultipleElectrolytesOrderingTests, Eval, EvalTypes)
{
    using Scalar = typename MathToolbox<Eval>::Scalar;
    using Electrolyte = typename SaltElectrolytes<Scalar>::Electrolyte;

    // Mg2+ pairs fully with SO4^2- into MgSO4, and K+ pairs fully with Cl- into KCl: two
    // electrolytes, no leftover molality
    SaltArray<Eval, SaltMolality> molal;
    molal[SaltIndex::MG] = 1.0;
    molal[SaltIndex::SO4] = 1.0;
    molal[SaltIndex::K] = 2.0;
    molal[SaltIndex::CL] = 2.0;
    SaltArray<Eval, SaltMassFraction> salinity = molal.template convert_to<SaltMassFraction>();

    auto electrolytes = SaltElectrolytes<Scalar>::fromSaltComponents(salinity);
    BOOST_REQUIRE_EQUAL(electrolytes.size(), 2U);

    // Electrolytes are generated anion-first in decreasing ion strength: SO4 (and its
    // MgSO4 pairing) before Cl (and its KCl pairing)
    BOOST_CHECK(electrolytes[0].first == Electrolyte::MgSO4);
    BOOST_CHECK(electrolytes[1].first == Electrolyte::KCl);

    // No residual molality: the two electrolyte mass fractions sum to the total
    // dissolved-salt mass fraction
    BOOST_CHECK_CLOSE(electrolytes[0].second + electrolytes[1].second,
                       salinity.sum(),
                       static_cast<Scalar>(1.0e-4));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnpairedIonsAreIgnoredTests, Eval, EvalTypes)
{
    using Scalar = typename MathToolbox<Eval>::Scalar;

    // Cations only, no anion present to pair with: no electrolyte can be formed and the
    // dissolved salt is ignored (with a log warning), not thrown as an error
    SaltArray<Eval, SaltMolality> molal;
    molal[SaltIndex::NA] = 1.0;
    molal[SaltIndex::CA] = 2.0;
    SaltArray<Eval, SaltMassFraction> salinity = molal.template convert_to<SaltMassFraction>();
    BOOST_REQUIRE(salinity.any_nonzero());

    auto electrolytes = SaltElectrolytes<Scalar>::fromSaltComponents(salinity);
    BOOST_CHECK(electrolytes.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(PartialRemainderIsIgnoredTests, Eval, EvalTypes)
{
    using Scalar = typename MathToolbox<Eval>::Scalar;
    using Electrolyte = typename SaltElectrolytes<Scalar>::Electrolyte;

    // Excess Na+ beyond what the available Cl- can pair with; only the paired part forms
    // NaCl, the leftover Na+ molality is ignored
    SaltArray<Eval, SaltMolality> molal;
    molal[SaltIndex::NA] = 3.0;
    molal[SaltIndex::CL] = 1.0;
    SaltArray<Eval, SaltMassFraction> salinity = molal.template convert_to<SaltMassFraction>();

    auto electrolytes = SaltElectrolytes<Scalar>::fromSaltComponents(salinity);
    BOOST_REQUIRE_EQUAL(electrolytes.size(), 1U);
    BOOST_CHECK(electrolytes[0].first == Electrolyte::NaCl);

    // Only 1 of the 3 molal Na+ could pair with Cl-, so the returned NaCl mass fraction is
    // strictly less than the total dissolved-salt mass fraction
    BOOST_CHECK(electrolytes[0].second < salinity.sum());
}
