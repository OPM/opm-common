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
 * \brief Baked-table evaluation of relative permeability and capillary
 *        pressure for the ECL-default three-phase material law.
 *
 * For each cell the installed material law (multiplexer params from the
 * EclMaterialLawManager, endpoint scaling included) is sampled onto minimal
 * exact piecewise-linear tables on three axes:
 *   Sw axis: [krw, pcow-diff]     (pcow-diff = pC[w]-pC[o])
 *   Sg axis: [krg, pcgo-diff]
 *   S  axis: [kro_ow, kro_go]     (S = max(Sw,Swco)+Sg; ECL default 3ph oil)
 * Exactness comes from bake-then-refine: candidate nodes are densified (with
 * kink snapping) until midpoint probes of every interval reproduce the true
 * law, then collinear nodes are pruned. Tables are deduplicated across cells
 * by the shape of the scaled EPS info; vertical endpoint scales (maxKr*,
 * maxPc*, e.g. from SWATINIT) are factored into per-cell linear multipliers.
 * Storage is a small table pool plus one index and a few scalars per cell.
 * Correctness is checked at build time against the true law (spotValidate).
 */
#ifndef OPM_BAKED_SATFUNC_TABLES_HPP
#define OPM_BAKED_SATFUNC_TABLES_HPP

#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidstates/SimpleModularFluidState.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace Opm {

// Minimal fluid state carrying only saturations (all the family-I curves read).
template <class Scalar, int numPhases>
struct SatOnlyFluidState
{
    std::array<Scalar, numPhases> sat{};
    Scalar saturation(unsigned phaseIdx) const
    { return sat[phaseIdx]; }
};

// One two-column curve with precomputed slopes; x strictly increasing on [0,1].
// Node values interleaved as {v0, s0, v1, s1} so an evaluation after the
// search touches a single cache line (the flowdiagnostics table layout).
struct BakedCurve
{
    std::vector<double> x;
    std::vector<double> vals;   // 4 doubles per node: v0, s0, v1, s1

    // exact search acceleration: a uniform hint grid maps the abscissa to a
    // starting node; the forward scan is bounded by nodes per hint cell.
    static constexpr std::size_t hintCells = 64;
    double x0 = 0.0;
    double invH = 0.0;
    std::array<std::uint16_t, hintCells + 1> hint{};

    void buildHint()
    {
        x0 = x.front();
        const double span = x.back() - x0;
        invH = (span > 0.0) ? double(hintCells) / span : 0.0;
        std::size_t i = 0;
        for (std::size_t j = 0; j <= hintCells; ++j) {
            const double xj = x0 + (invH > 0.0 ? double(j) / invH : 0.0);
            while (i + 2 < x.size() && x[i + 1] <= xj) {
                ++i;
            }
            hint[j] = static_cast<std::uint16_t>(i);
        }
    }

    // one hint lookup + short scan serves both columns; derivatives analytic
    template <class Evaluation>
    void eval2(const Evaluation& xe, Evaluation& c0, Evaluation& c1) const
    {
        double xd;
        if constexpr (std::is_floating_point_v<Evaluation>) {
            xd = xe;
        }
        else {
            xd = xe.value();
        }
        const double* xb = x.data();
        const double cell = std::min(double(hintCells), std::max(0.0, (xd - x0) * invH));
        std::size_t lo = hint[static_cast<std::size_t>(cell)];
        while (lo + 2 < x.size() && xb[lo + 1] <= xd) {
            ++lo;
        }
        const double dx = xd - xb[lo];
        const double* nd = vals.data() + 4 * lo;
        if constexpr (std::is_floating_point_v<Evaluation>) {
            c0 = nd[0] + nd[1] * dx;
            c1 = nd[2] + nd[3] * dx;
        }
        else {
            c0.setValue(nd[0] + nd[1] * dx);
            c1.setValue(nd[2] + nd[3] * dx);
            for (int k = 0; k < xe.size(); ++k) {
                c0.setDerivative(k, nd[1] * xe.derivative(k));
                c1.setDerivative(k, nd[3] * xe.derivative(k));
            }
        }
    }
};

struct BakedTable
{
    BakedCurve sw;   // krw, pcow-diff
    BakedCurve sg;   // krg, pcgo-diff
    BakedCurve so;   // kro_ow, kro_go   (abscissa is S = sw+sg)
    double swco = 0.0;
};

template <class MaterialLaw, class Manager, class Scalar>
class BakedSatfuncTables
{
    using MultiplexerParams = typename Manager::MaterialLawParams;
    using DefaultMaterial = typename MaterialLaw::DefaultMaterial;
    using DefaultParams = typename DefaultMaterial::Params;
    static constexpr int numPhases = 3;
    static constexpr unsigned waterIdx = MaterialLaw::Traits::wettingPhaseIdx;
    static constexpr unsigned oilIdx = MaterialLaw::Traits::nonWettingPhaseIdx;
    static constexpr unsigned gasIdx = MaterialLaw::Traits::gasPhaseIdx;

    using FS = SatOnlyFluidState<Scalar, numPhases>;

public:
    // truth functions of one variable each (see header comment)
    struct Truth
    {
        const DefaultParams* p;
        double swco;

        std::array<double, 2> swAxis(double swv) const
        {   // krw, pcow-diff at (Sw=swv, Sg=0)
            FS fs; fs.sat[waterIdx] = swv; fs.sat[oilIdx] = 1.0 - swv; fs.sat[gasIdx] = 0.0;
            std::array<double, numPhases> pC{};
            DefaultMaterial::capillaryPressures(pC, *p, fs);
            const double krw = DefaultMaterial::template krw<FS, double>(*p, fs);
            return { krw, pC[waterIdx] - pC[oilIdx] };
        }
        std::array<double, 2> sgAxis(double sgv) const
        {   // krg, pcgo-diff at (Sw=swco, Sg=sgv)
            FS fs; fs.sat[waterIdx] = swco; fs.sat[gasIdx] = sgv;
            fs.sat[oilIdx] = 1.0 - swco - sgv;
            std::array<double, numPhases> pC{};
            DefaultMaterial::capillaryPressures(pC, *p, fs);
            const double krg = DefaultMaterial::template krg<FS, double>(*p, fs);
            return { krg, pC[gasIdx] - pC[oilIdx] };
        }
        std::array<double, 2> soAxis(double S) const
        {   // kro_ow(S) at (Sw=S, Sg=0); kro_go(S) at (Sw=swco, Sg=S-swco)
            FS a; a.sat[waterIdx] = S; a.sat[oilIdx] = 1.0 - S; a.sat[gasIdx] = 0.0;
            FS b; b.sat[waterIdx] = swco; b.sat[gasIdx] = std::max(S - swco, 0.0);
            b.sat[oilIdx] = 1.0 - swco - b.sat[gasIdx];
            const double kroOw = DefaultMaterial::template relpermOilInOilWaterSystem<double, FS>(*p, a);
            const double kroGo = DefaultMaterial::template relpermOilInOilGasSystem<double, FS>(*p, b);
            return { kroOw, kroGo };
        }
    };

    // per-cell factor slots: [krw, kro_ow, pcow, krg, kro_go, pcgo]
    static constexpr int numFactors = 6;

    void build(const Manager& manager, std::size_t nc)
    {
        cellTable_.resize(nc);
        cellSwco_.resize(nc);
        cellFactors_.assign(numFactors * nc, 1.0);
        std::map<std::vector<std::int64_t>, std::uint32_t> dedup;
        std::vector<std::array<double, numFactors>> tableRefs;

        for (std::size_t c = 0; c < nc; ++c) {
            const auto& info = manager.oilWaterScaledEpsInfoDrainage(c);
            const int satnum = manager.satnumRegionIdx(c);
            // Vertical endpoint scales (maxKr*, maxPc*) are NOT part of the
            // key: 2pt vertical scaling — and 3pt with equal Kr*r/maxKr*
            // ratios, which the key preserves — is linear in the ordinate and
            // factored into per-cell multipliers. Without this, per-cell pc
            // (SWATINIT) or kr scaling degenerates to one table per cell.
            const auto key = makeKey(satnum, info,
                                     manager.oilWaterConfig(), manager.gasOilConfig());
            const std::array<double, numFactors> refs =
                { info.maxKrw, info.maxKrow, info.maxPcow,
                  info.maxKrg, info.maxKrog, info.maxPcgo };
            auto [it, inserted] = dedup.try_emplace(key, static_cast<std::uint32_t>(tables_.size()));
            if (inserted) {
                tables_.push_back(bakeOne(manager, c, info.Swl));
                tableRefs.push_back(refs);
            }
            cellTable_[c] = it->second;
            cellSwco_[c] = tables_[it->second].swco;
            for (int k = 0; k < numFactors; ++k) {
                cellFactors_[numFactors * c + k] = linFactor(refs[k], tableRefs[it->second][k]);
            }
        }

        spotValidate(manager, nc);
    }

    std::size_t numTables() const { return tables_.size(); }
    std::size_t numNodes() const
    {
        std::size_t n = 0;
        for (const auto& t : tables_) {
            n += t.sw.x.size() + t.sg.x.size() + t.so.x.size();
        }
        return n;
    }
    double maxBakeError() const { return maxBakeError_; }

    // Fused evaluation: kr[3], pcow-diff, pcgo-diff. One search per axis,
    // analytic derivatives, ECL-default 3-phase oil combine (with the same
    // epsilon regularization as EclDefaultMaterial::krn).
    template <class Evaluation>
    void evaluate(std::size_t cellIdx,
                  const Evaluation& sw,
                  const Evaluation& sg,
                  std::array<Evaluation, 3>& kr,     // [water, oil, gas] canonical idx below
                  Evaluation& pcowDiff,
                  Evaluation& pcgoDiff) const
    {
        const BakedTable& t = tables_[cellTable_[cellIdx]];
        const double swco = cellSwco_[cellIdx];
        const double* f = &cellFactors_[numFactors * cellIdx];

        Evaluation krw, krg, kroOw, kroGo;
        t.sw.eval2(sw, krw, pcowDiff);
        krw *= f[0];
        pcowDiff *= f[2];
        t.sg.eval2(sg, krg, pcgoDiff);
        krg *= f[3];
        pcgoDiff *= f[5];

        const Evaluation swClamped = max(Evaluation(swco), sw);
        const Evaluation S = sg + swClamped;
        t.so.eval2(S, kroOw, kroGo);
        kroOw *= f[1];   // factors applied before the three-phase combine
        kroGo *= f[4];

        Evaluation kro;
        constexpr double epsilon = 1e-5;
        const double Sval = Opm::getValue(S);
        if (Sval - swco < epsilon) {
            const Evaluation kro2 = (kroOw + kroGo) / 2;
            if (Sval - swco > epsilon / 2) {
                const Evaluation kro1 = (sg * kroGo + (swClamped - swco) * kroOw) / (S - swco);
                const Evaluation alpha = (epsilon - (S - swco)) / (epsilon / 2);
                kro = kro2 * alpha + kro1 * (1 - alpha);
            }
            else {
                kro = kro2;
            }
        }
        else {
            kro = (sg * kroGo + (swClamped - swco) * kroOw) / (S - swco);
        }

        kr[waterIdx] = krw;
        kr[oilIdx] = kro;
        kr[gasIdx] = krg;
    }

private:
    static double linFactor(double cellV, double tableV)
    {
        if (tableV > 0.0 && cellV > 0.0) {
            const double fac = cellV / tableV;
            if (std::isfinite(fac)) {
                return fac;
            }
        }
        return 1.0;
    }

    template <class Info, class EpsConfig>
    static std::vector<std::int64_t> makeKey(int satnum, const Info& i,
                                             const EpsConfig& owCfg, const EpsConfig& goCfg)
    {
        const auto q = [](double v) { return static_cast<std::int64_t>(std::llround(v * 1.0e9)); };
        std::vector<std::int64_t> key
            { satnum,
              q(i.Swl), q(i.Swcr), q(i.Swu), q(i.Sgl), q(i.Sgcr), q(i.Sgu),
              q(i.Sowcr), q(i.Sogcr) };
        // Vertical scales enter only as shape information: with 3pt vertical
        // scaling active the residual/max ratio shapes the curve; with plain
        // 2pt scaling the magnitude is a pure per-cell factor and does not
        // key. A non-positive max cannot be factored and keys the raw pair.
        const auto shape = [&](double r, double m, bool threePt) {
            if (m > 0.0) {
                key.push_back(threePt ? q(r / m) : 0);
            }
            else {
                key.push_back(std::numeric_limits<std::int64_t>::min());
                key.push_back(q(r));
                key.push_back(q(m));
            }
        };
        // gas-oil system: wetting = oil (kro_go), non-wetting = gas (krg)
        shape(i.Krwr, i.maxKrw, owCfg.enableThreePointKrwScaling());
        shape(i.Krorw, i.maxKrow, owCfg.enableThreePointKrnScaling());
        shape(0.0, i.maxPcow, false);
        shape(i.Krgr, i.maxKrg, goCfg.enableThreePointKrnScaling());
        shape(i.Krorg, i.maxKrog, goCfg.enableThreePointKrwScaling());
        shape(0.0, i.maxPcgo, false);
        return key;
    }

    // Compare the baked evaluation (incl. the factored per-cell pc scale)
    // against the true material law on a sample of cells and states.
    void spotValidate(const Manager& manager, std::size_t nc)
    {
        const std::size_t stride = std::max<std::size_t>(nc / 200, 1);
        for (std::size_t c = 0; c < nc; c += stride) {
            const auto& mp = manager.materialLawParams(c);
            const DefaultParams& dp =
                mp.template getRealParams<Opm::EclMultiplexerApproach::Default>();
            for (int a = 0; a <= 3; ++a) {
                for (int b = 0; a + b <= 3; ++b) {
                    const double sw = 0.05 + 0.3 * a;
                    const double sg = 0.02 + 0.3 * b;
                    FS fs;
                    fs.sat[waterIdx] = sw;
                    fs.sat[gasIdx] = sg;
                    fs.sat[oilIdx] = 1.0 - sw - sg;
                    std::array<double, numPhases> krT{}, pcT{};
                    DefaultMaterial::relativePermeabilities(krT, dp, fs);
                    DefaultMaterial::capillaryPressures(pcT, dp, fs);
                    std::array<double, 3> krB;
                    double pcow, pcgo;
                    evaluate(c, sw, sg, krB, pcow, pcgo);
                    double err = 0.0;
                    for (int ph = 0; ph < numPhases; ++ph) {
                        err = std::max(err, std::abs(krB[ph] - krT[ph])
                                            / std::max(std::abs(krT[ph]), 1.0));
                    }
                    const double pcowT = pcT[waterIdx] - pcT[oilIdx];
                    const double pcgoT = pcT[gasIdx] - pcT[oilIdx];
                    err = std::max(err, std::abs(pcow - pcowT) / std::max(std::abs(pcowT), 1.0));
                    err = std::max(err, std::abs(pcgo - pcgoT) / std::max(std::abs(pcgoT), 1.0));
                    maxBakeError_ = std::max(maxBakeError_, err);
                }
            }
        }
    }

    template <class Fn>
    BakedCurve bakeCurve(const Fn& truth, std::set<double>& nodes)
    {
        // refine until every interval's midpoint matches the true law
        constexpr double tol = 1.0e-12;
        for (int pass = 0; pass < 12; ++pass) {
            std::vector<double> xs(nodes.begin(), nodes.end());
            std::vector<double> insert;
            for (std::size_t i = 0; i + 1 < xs.size(); ++i) {
                const double xm = 0.5 * (xs[i] + xs[i + 1]);
                if (xs[i + 1] - xs[i] < 1.0e-10) {
                    continue;
                }
                const auto lo = truth(xs[i]);
                const auto hi = truth(xs[i + 1]);
                const auto mid = truth(xm);
                for (int col = 0; col < 2; ++col) {
                    const double lin = 0.5 * (lo[col] + hi[col]);
                    const double scale = std::max({ std::abs(lo[col]), std::abs(hi[col]), 1.0 });
                    if (std::abs(mid[col] - lin) > tol * scale) {
                        insert.push_back(xm);
                        // kink snap: for piecewise-linear truth with one kink in
                        // the interval, intersecting the secants through the
                        // left and right quarters yields the kink exactly.
                        const double w = xs[i + 1] - xs[i];
                        const double xq1 = xs[i] + 0.25 * w;
                        const double xq3 = xs[i] + 0.75 * w;
                        const auto q1 = truth(xq1);
                        const auto q3 = truth(xq3);
                        const double sl = (q1[col] - lo[col]) / (xq1 - xs[i]);
                        const double sr = (hi[col] - q3[col]) / (xs[i + 1] - xq3);
                        if (std::abs(sl - sr) > 1.0e-14 * std::max(std::abs(sl), std::abs(sr))) {
                            // lines: lo + sl (x - xi)  and  hi + sr (x - xi1)
                            const double xk = (hi[col] - lo[col] + sl * xs[i] - sr * xs[i + 1])
                                              / (sl - sr);
                            if (xk > xs[i] + 1.0e-12 && xk < xs[i + 1] - 1.0e-12) {
                                insert.push_back(xk);
                            }
                        }
                        break;
                    }
                }
            }
            if (insert.empty()) {
                break;
            }
            nodes.insert(insert.begin(), insert.end());
        }

        // sample final nodes, then prune collinear interior nodes
        std::vector<double> xs(nodes.begin(), nodes.end());
        std::vector<std::array<double, 2>> ys(xs.size());
        for (std::size_t i = 0; i < xs.size(); ++i) {
            ys[i] = truth(xs[i]);
        }
        std::vector<char> keep(xs.size(), 1);
        for (std::size_t i = 1; i + 1 < xs.size(); ++i) {
            bool collinear = true;
            // find previous kept node
            std::size_t p = i - 1;
            while (!keep[p]) { --p; }
            const double t = (xs[i] - xs[p]) / (xs[i + 1] - xs[p]);
            for (int col = 0; col < 2 && collinear; ++col) {
                const double lin = ys[p][col] * (1.0 - t) + ys[i + 1][col] * t;
                const double scale = std::max(std::abs(ys[i][col]), 1.0);
                collinear = std::abs(lin - ys[i][col]) <= 1.0e-13 * scale;
            }
            if (collinear) {
                keep[i] = 0;
            }
        }

        BakedCurve c;
        std::vector<std::array<double, 2>> kv;
        for (std::size_t i = 0; i < xs.size(); ++i) {
            if (keep[i]) {
                c.x.push_back(xs[i]);
                kv.push_back(ys[i]);
            }
        }
        const std::size_t n = c.x.size();
        c.vals.assign(4 * n, 0.0);
        for (std::size_t i = 0; i < n; ++i) {
            const double idx = (i + 1 < n) ? 1.0 / (c.x[i + 1] - c.x[i]) : 0.0;
            c.vals[4 * i + 0] = kv[i][0];
            c.vals[4 * i + 1] = (i + 1 < n) ? (kv[i + 1][0] - kv[i][0]) * idx : 0.0;
            c.vals[4 * i + 2] = kv[i][1];
            c.vals[4 * i + 3] = (i + 1 < n) ? (kv[i + 1][1] - kv[i][1]) * idx : 0.0;
        }

        // validation probe (golden-ratio points; belt and braces)
        double maxErr = 0.0;
        for (std::size_t i = 0; i + 1 < n; ++i) {
            const double xp = c.x[i] + 0.618033988749895 * (c.x[i + 1] - c.x[i]);
            const auto tv = truth(xp);
            const double dx = xp - c.x[i];
            const double p0 = c.vals[4 * i + 0] + c.vals[4 * i + 1] * dx;
            const double p1 = c.vals[4 * i + 2] + c.vals[4 * i + 3] * dx;
            maxErr = std::max(maxErr, std::abs(p0 - tv[0]) / std::max(std::abs(tv[0]), 1.0));
            maxErr = std::max(maxErr, std::abs(p1 - tv[1]) / std::max(std::abs(tv[1]), 1.0));
        }
        maxBakeError_ = std::max(maxBakeError_, maxErr);
        c.buildHint();
        return c;
    }

    BakedTable bakeOne(const Manager& manager, std::size_t cellIdx, double swco)
    {
        const auto& mp = manager.materialLawParams(cellIdx);
        const DefaultParams& dp =
            mp.template getRealParams<Opm::EclMultiplexerApproach::Default>();
        Truth truth{ &dp, swco };

        // candidate nodes: endpoints + scaled EPS anchors + uniform fill;
        // the refine loop discovers any interior table kinks.
        const auto& info = manager.oilWaterScaledEpsInfoDrainage(cellIdx);
        const auto seed = [&](std::initializer_list<double> anchors) {
            std::set<double> s;
            for (int i = 0; i <= 64; ++i) {
                s.insert(double(i) / 64.0);
            }
            for (double a : anchors) {
                if (a > 0.0 && a < 1.0) {
                    s.insert(a);
                }
            }
            return s;
        };

        BakedTable t;
        t.swco = swco;
        {
            auto nodes = seed({ info.Swl, info.Swcr, info.Swu, 1.0 - info.Sowcr,
                                1.0 - info.Sowcr - info.Sgl });
            t.sw = bakeCurve([&](double x) { return truth.swAxis(x); }, nodes);
        }
        {
            auto nodes = seed({ info.Sgl, info.Sgcr, info.Sgu, 1.0 - info.Sogcr - info.Swl });
            t.sg = bakeCurve([&](double x) { return truth.sgAxis(x); }, nodes);
        }
        {
            auto nodes = seed({ info.Swl, info.Swcr, 1.0 - info.Sowcr, info.Swl + info.Sgcr,
                                1.0 - info.Sogcr });
            t.so = bakeCurve([&](double x) { return truth.soAxis(x); }, nodes);
        }
        return t;
    }

    std::vector<BakedTable> tables_;
    std::vector<std::uint32_t> cellTable_;
    std::vector<double> cellSwco_;
    std::vector<double> cellFactors_;   // numFactors per cell
    double maxBakeError_ = 0.0;
};

} // namespace Opm

#endif
