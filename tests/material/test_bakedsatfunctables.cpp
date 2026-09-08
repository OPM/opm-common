// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 Equinor ASA.

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
/*!
 * \file
 * \brief Exactness and deduplication tests for BakedSatfuncTables: the baked
 *        piecewise-linear representation must reproduce the installed
 *        material law (endpoint scaling included) to near machine precision,
 *        and per-cell vertical scales must be factored, not multiply tables.
 */
#include "config.h"

#define BOOST_TEST_MODULE BakedSatfuncTables
#include <boost/test/unit_test.hpp>

#include <opm/material/fluidmatrixinteractions/BakedSatfuncTables.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <array>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace {

const std::string commonDeck =
    "DIMENS\n"
    "   10 10 3 /\n"
    "\n"
    "TABDIMS\n"
    "/\n"
    "\n"
    "OIL\n"
    "GAS\n"
    "WATER\n"
    "\n"
    "DISGAS\n"
    "\n"
    "FIELD\n"
    "\n"
    "GRID\n"
    "\n"
    "DX\n"
    "       300*1000 /\n"
    "DY\n"
    "   300*1000 /\n"
    "DZ\n"
    "   100*20 100*30 100*50 /\n"
    "\n"
    "TOPS\n"
    "   100*8325 /\n"
    "\n"
    "PORO\n"
    "  300*0.15 /\n"
    "PROPS\n"
    "\n"
    "SWOF\n"
    "0.12   0               1   0\n"
    "0.18   4.64876033057851E-008   1   0\n"
    "0.24   0.000000186     0.997   0\n"
    "0.3    4.18388429752066E-007   0.98    0.1\n"
    "0.36   7.43801652892562E-007   0.7 0.2\n"
    "0.42   1.16219008264463E-006   0.35    0.32\n"
    "0.48   1.67355371900826E-006   0.2 0.6\n"
    "0.54   2.27789256198347E-006   0.09    1.2\n"
    "0.6    2.97520661157025E-006   0.021   2.5\n"
    "0.66   3.7654958677686E-006    0.01    3.5\n"
    "0.72   4.64876033057851E-006   0.001   4.0\n"
    "0.78   0.000005625     0.0001  4.5\n"
    "0.84   6.69421487603306E-006   0   5.0\n"
    "0.91   8.05914256198347E-006   0   6.0\n"
    "1      0.984           0   7.0 /\n"
    "\n"
    "SGOF\n"
    "0  0   1   0\n"
    "0.001  0   1   0\n"
    "0.02   0   0.997   0.1\n"
    "0.05   0.005   0.980   0.2\n"
    "0.12   0.025   0.700   0.4\n"
    "0.2    0.075   0.350   0.6\n"
    "0.25   0.125   0.200   0.7\n"
    "0.3    0.190   0.090   0.9\n"
    "0.4    0.410   0.021   1.2\n"
    "0.45   0.60    0.010   1.5\n"
    "0.5    0.72    0.001   1.9\n"
    "0.6    0.87    0.0001  2.4\n"
    "0.7    0.94    0.000   3.0\n"
    "0.85   0.98    0.000   3.5\n"
    "0.88   0.984   0.000   4.0 /\n";

const std::string fam1DeckString = "RUNSPEC\n\n" + commonDeck;

// Endpoint scaling with two SWL groups; KRW/PCW vary in four groups so the
// bake must factor them (linear vertical scaling) instead of keying them.
const std::string epsDeckString =
    "RUNSPEC\n"
    "\n"
    "ENDSCALE\n"
    "/\n\n"
    + commonDeck +
    "\n"
    "SWL\n"
    "  150*0.12 150*0.16 /\n"
    "KRW\n"
    "  75*0.984 75*0.7 75*0.5 75*0.35 /\n"
    "PCW\n"
    "  75*7.0 75*5.0 75*3.0 75*2.0 /\n";

template <class Scalar>
struct Fixture
{
    enum { numPhases = 3 };
    enum { waterPhaseIdx = 0 };
    enum { oilPhaseIdx = 1 };
    enum { gasPhaseIdx = 2 };
    using MaterialTraits = Opm::ThreePhaseMaterialTraits<Scalar,
                                                         waterPhaseIdx,
                                                         oilPhaseIdx,
                                                         gasPhaseIdx,
                                                         /*enableHysteresis=*/false,
                                                         /*enableEndpointScaling=*/true>;
    using MaterialLawManager = Opm::EclMaterialLaw::Manager<MaterialTraits>;
    using MaterialLaw = typename MaterialLawManager::MaterialLaw;
    using Baked = Opm::BakedSatfuncTables<MaterialLaw, MaterialLawManager, Scalar>;
};

std::function<std::vector<int>(const Opm::FieldPropsManager&, const std::string&, bool)> lookup =
    [](const Opm::FieldPropsManager& fp, const std::string& prop, bool translate)
    {
        std::vector<int> dest;
        const auto& raw = fp.get_int(prop);
        dest.resize(raw.size());
        for (std::size_t i = 0; i < raw.size(); ++i) {
            dest[i] = raw[i] - static_cast<int>(translate);
        }
        return dest;
    };

std::function<unsigned(unsigned)> identity = [](unsigned i) { return i; };

// Compare the baked evaluation against the installed law on a saturation
// sweep for the given cells.
template <class Scalar>
void checkExactness(const typename Fixture<Scalar>::MaterialLawManager& manager,
                    const typename Fixture<Scalar>::Baked& baked,
                    const std::vector<std::size_t>& cells)
{
    using Fix = Fixture<Scalar>;
    using MaterialLaw = typename Fix::MaterialLaw;
    using DefaultMaterial = typename MaterialLaw::DefaultMaterial;
    using FS = Opm::SatOnlyFluidState<Scalar, Fix::numPhases>;

    double maxErr = 0.0;
    std::array<double, 3> worst{};
    for (const std::size_t c : cells) {
        const auto& mp = manager.materialLawParams(c);
        const auto& dp = mp.template getRealParams<Opm::EclMultiplexerApproach::Default>();
        for (int a = 0; a <= 20; ++a) {
            for (int b = 0; a + b <= 20; ++b) {
                const Scalar sw = Scalar(a) / 20.0;
                const Scalar sg = Scalar(b) / 20.0;
                FS fs;
                fs.sat[Fix::waterPhaseIdx] = sw;
                fs.sat[Fix::gasPhaseIdx] = sg;
                fs.sat[Fix::oilPhaseIdx] = 1.0 - sw - sg;

                std::array<Scalar, Fix::numPhases> krT{}, pcT{};
                DefaultMaterial::relativePermeabilities(krT, dp, fs);
                DefaultMaterial::capillaryPressures(pcT, dp, fs);

                std::array<Scalar, 3> krB;
                Scalar pcow, pcgo;
                baked.evaluate(c, sw, sg, krB, pcow, pcgo);

                const auto relErr = [](Scalar bakedV, Scalar trueV) {
                    if (!std::isfinite(double(trueV))) {
                        return 0.0;   // outside the law's domain; nothing to match
                    }
                    return std::abs(double(bakedV - trueV))
                        / std::max(std::abs(double(trueV)), 1.0);
                };
                double err = 0.0;
                for (int ph = 0; ph < Fix::numPhases; ++ph) {
                    err = std::max(err, relErr(krB[ph], krT[ph]));
                }
                const Scalar pcowT = pcT[Fix::waterPhaseIdx] - pcT[Fix::oilPhaseIdx];
                const Scalar pcgoT = pcT[Fix::gasPhaseIdx] - pcT[Fix::oilPhaseIdx];
                err = std::max(err, relErr(pcow, pcowT));
                err = std::max(err, relErr(pcgo, pcgoT));
                if (err > maxErr) {
                    maxErr = err;
                    worst = { double(sw), double(sg), double(c) };
                }
            }
        }
    }
    BOOST_TEST_MESSAGE("max rel err " << maxErr << " at sw=" << worst[0]
                       << " sg=" << worst[1] << " cell=" << worst[2]);
    BOOST_CHECK_MESSAGE(maxErr < 1e-8,
                        "baked vs law mismatch: max rel err " << maxErr
                        << " at sw=" << worst[0] << " sg=" << worst[1]
                        << " cell=" << worst[2]);
}

} // namespace

BOOST_AUTO_TEST_CASE(UnscaledFamilyOneIsExactAndDeduplicates)
{
    using Fix = Fixture<double>;

    Opm::Parser parser;
    const auto deck = parser.parseString(fam1DeckString);
    const Opm::EclipseState eclState(deck);
    const std::size_t n = eclState.getInputGrid().getNumActive();

    typename Fix::MaterialLawManager manager;
    manager.initFromState(eclState);
    manager.initParamsForElements(eclState, n, lookup, identity);

    typename Fix::Baked baked;
    baked.build(manager, n);

    BOOST_CHECK_EQUAL(baked.numTables(), 1U);
    BOOST_CHECK_LT(baked.maxBakeError(), 1e-9);
    checkExactness<double>(manager, baked, {0, n / 2, n - 1});
}

BOOST_AUTO_TEST_CASE(VerticalScalingIsFactoredNotKeyed)
{
    using Fix = Fixture<double>;

    Opm::Parser parser;
    const auto deck = parser.parseString(epsDeckString);
    const Opm::EclipseState eclState(deck);
    const std::size_t n = eclState.getInputGrid().getNumActive();

    typename Fix::MaterialLawManager manager;
    manager.initFromState(eclState);
    manager.initParamsForElements(eclState, n, lookup, identity);

    typename Fix::Baked baked;
    baked.build(manager, n);

    // two SWL groups shape the curves; the four KRW/PCW groups must be
    // absorbed by the per-cell linear factors
    BOOST_CHECK_EQUAL(baked.numTables(), 2U);
    BOOST_CHECK_LT(baked.maxBakeError(), 1e-9);

    std::vector<std::size_t> cells;
    for (std::size_t c = 0; c < n; c += 37) {
        cells.push_back(c);
    }
    checkExactness<double>(manager, baked, cells);
}
