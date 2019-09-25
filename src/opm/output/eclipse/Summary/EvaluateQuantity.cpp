/*
  Copyright (c) 2019 Equinor ASA

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

#include <opm/output/eclipse/Summary/EvaluateQuantity.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group2.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Connection.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/RegionCache.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

    template <Opm::UnitSystem::measure m>
    struct UnitTag {};

    template <Opm::UnitSystem::measure m>
    inline constexpr Opm::UnitSystem::measure unitOfMeasure(UnitTag<m>)
    {
        return m;
    }

    namespace Dynamic
    {
        template <Opm::data::Rates::opt phase, bool polymer>
        struct Rate
        {
            using unit = UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>;
        };

        template <Opm::data::Rates::opt phase>
        struct Rate<phase, true>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::mass_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::gas, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::gas_surface_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::solvent, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::gas_surface_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::vaporized_oil, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::dissolved_gas, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::gas_surface_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::well_potential_water, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::well_potential_oil, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::well_potential_gas, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::gas_surface_rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::reservoir_water, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::reservoir_oil, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::rate>;
        };

        template <bool polymer>
        struct Rate<Opm::data::Rates::opt::reservoir_gas, polymer>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::rate>;
        };

        template <Opm::data::Rates::opt phase>
        struct ProdIndex;

        template <>
        struct ProdIndex<Opm::data::Rates::opt::productivity_index_oil>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::liquid_productivity_index>;
        };

        template <>
        struct ProdIndex<Opm::data::Rates::opt::productivity_index_water>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::liquid_productivity_index>;
        };

        template <>
        struct ProdIndex<Opm::data::Rates::opt::productivity_index_gas>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::gas_productivity_index>;
        };
    } // namespace Dynamic

    namespace Declared
    {
        template <Opm::Phase phase>
        struct Rate
        {
            using unit = UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>;
        };

        template <>
        struct Rate<Opm::Phase::GAS>
        {
            using unit = UnitTag<Opm::UnitSystem::measure::gas_surface_rate>;
        };
    } // namespace Declared

    template <Opm::UnitSystem::measure m>
    inline constexpr Opm::UnitSystem::measure
    operator+(UnitTag<m> lhs, UnitTag<m>)
    {
        return unitOfMeasure(lhs);
    }

    template <Opm::UnitSystem::measure m>
    inline constexpr Opm::UnitSystem::measure
    operator-(UnitTag<m> lhs, UnitTag<m>)
    {
        return unitOfMeasure(lhs);
    }

    inline constexpr Opm::UnitSystem::measure
    operator*(UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>,
              UnitTag<Opm::UnitSystem::measure::time>)
    {
        return Opm::UnitSystem::measure::liquid_surface_volume;
    }

    inline constexpr Opm::UnitSystem::measure
    operator*(UnitTag<Opm::UnitSystem::measure::gas_surface_rate>,
              UnitTag<Opm::UnitSystem::measure::time>)
    {
        return Opm::UnitSystem::measure::gas_surface_volume;
    }

    inline constexpr Opm::UnitSystem::measure
    operator*(UnitTag<Opm::UnitSystem::measure::rate>,
              UnitTag<Opm::UnitSystem::measure::time>)
    {
        return Opm::UnitSystem::measure::volume;
    }

    inline constexpr Opm::UnitSystem::measure
    operator*(UnitTag<Opm::UnitSystem::measure::mass_rate>,
              UnitTag<Opm::UnitSystem::measure::time>)
    {
        return Opm::UnitSystem::measure::mass;
    }

    inline constexpr Opm::UnitSystem::measure
    operator/(UnitTag<Opm::UnitSystem::measure::gas_surface_rate>,
              UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>)
    {
        return Opm::UnitSystem::measure::gas_oil_ratio;
    }

    inline constexpr Opm::UnitSystem::measure
    operator/(UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>,
              UnitTag<Opm::UnitSystem::measure::gas_surface_rate>)
    {
        return Opm::UnitSystem::measure::gas_oil_ratio;
    }

    inline constexpr Opm::UnitSystem::measure
    operator/(UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>,
              UnitTag<Opm::UnitSystem::measure::liquid_surface_rate>)
    {
        return Opm::UnitSystem::measure::water_cut;
    }

    template <Opm::UnitSystem::measure m>
    inline constexpr Opm::UnitSystem::measure
    operator/(UnitTag<m>, UnitTag<m>)
    {
        return Opm::UnitSystem::measure::identity;
    }

    inline Opm::UnitSystem::measure
    product(const Opm::UnitSystem::measure lhs,
            const Opm::UnitSystem::measure rhs)
    {
        using M = Opm::UnitSystem::measure;

        if ((lhs != M::time) && (rhs != M::time))
        {
            throw std::invalid_argument { "One unit must be time" };
        }

        const auto time  = UnitTag<M::time>{};
        const auto other = lhs == M::time ? rhs : lhs;
        switch (other) {
            case M::liquid_surface_rate:
                return UnitTag<M::liquid_surface_rate>{} * time;

            case M::gas_surface_rate:
                return UnitTag<M::gas_surface_rate>{} * time;

            case M::rate:
                return UnitTag<M::rate>{} * time;

            case M::mass_rate:
                return UnitTag<M::mass_rate>{} * time;
        }

        throw std::invalid_argument {
            "Unsupported unit of measurement: " +
            std::to_string(static_cast<int>(other))
        };
    }

    inline Opm::UnitSystem::measure
    quotient(const Opm::UnitSystem::measure numerator,
             const Opm::UnitSystem::measure denominator)
    {
        using M = Opm::UnitSystem::measure;

        if ((numerator   == M::gas_surface_rate) &&
            (denominator == M::liquid_surface_rate))
        {
            return UnitTag<M::gas_surface_rate>{}
                 / UnitTag<M::liquid_surface_rate>{};
        }

        if ((numerator   == M::liquid_surface_rate) &&
            (denominator == M::gas_surface_rate))
        {
            return UnitTag<M::liquid_surface_rate>{}
                 / UnitTag<M::gas_surface_rate>{};
        }

        if ((numerator   == M::liquid_surface_rate) &&
            (denominator == M::liquid_surface_rate))
        {
            return UnitTag<M::liquid_surface_rate>{}
                 / UnitTag<M::liquid_surface_rate>{};
        }

        throw std::invalid_argument {
            "Unsupported unit combination for quotient: " +
            std::to_string(static_cast<int>(numerator)) + '/' +
            std::to_string(static_cast<int>(denominator))
        };
    }

    constexpr double
    quotient(const double numerator, const double denominator)
    {
        return (std::abs(denominator) > 0.0)
            ? numerator / denominator
            : 0.0;
    }

} // namespace anonymous

namespace Opm { namespace SummaryHelpers {

    inline constexpr SummaryQuantity
    operator+(const SummaryQuantity& lhs, const SummaryQuantity& rhs)
    {
        //assert (lhs.unit == rhs.unit);

        return {lhs.value + rhs.value, lhs.unit};
    }

    inline constexpr SummaryQuantity
    operator-(const SummaryQuantity& lhs, const SummaryQuantity& rhs)
    {
        //assert (lhs.unit == rhs.unit);

        return {lhs.value - rhs.value, lhs.unit};
    }

    inline SummaryQuantity
    operator*(const SummaryQuantity& lhs, const SummaryQuantity& rhs)
    {
        return {
            lhs.value * rhs.value,
            product(lhs.unit, rhs.unit)
        };
    }

    inline SummaryQuantity
    operator/(const SummaryQuantity& lhs, const SummaryQuantity& rhs)
    {
        return {
            quotient(lhs.value, rhs.value),
            quotient(lhs.unit , rhs.unit)
        };
    }

    template <typename BinOp>
    class Combine
    {
    public:
        Combine(Evaluator f, Evaluator g)
            : f_(std::move(f))
            , g_(std::move(g))
        {}

        SummaryQuantity operator()(const EvaluationArguments& args) const
        {
            return BinOp{}(this->f_(args), this->g_(args));
        }

    private:
        Evaluator f_;
        Evaluator g_;
    };

    inline Combine<std::plus<SummaryQuantity>>
    operator+(Evaluator first, Evaluator second)
    {
        return { std::move(first), std::move(second) };
    }

    inline Combine<std::minus<SummaryQuantity>>
    operator-(Evaluator first, Evaluator second)
    {
        return { std::move(first), std::move(second) };
    }

    inline Combine<std::multiplies<SummaryQuantity>>
    operator*(Evaluator first, Evaluator second)
    {
        return { std::move(first), std::move(second) };
    }

    inline Combine<std::divides<SummaryQuantity>>
    operator/(Evaluator first, Evaluator second)
    {
        return { std::move(first), std::move(second) };
    }
}} // namespace Opm::SummaryHelpers

namespace {

    template <class ConnOp>
    Opm::SummaryHelpers::SummaryQuantity
    connectionResultQuantity(const Opm::SummaryHelpers::EvaluationArguments& args,
                             Opm::SummaryHelpers::SummaryQuantity            retval,
                             ConnOp&&                                        connOp)
    {
        if (args.schedule_wells.empty()) {
            return retval;
        }

        const auto& wname = args.schedule_wells.front();

        auto xwPos = args.well_sol.find(wname);
        if (xwPos == args.well_sol.end()) {
            return retval;
        }

        const auto& xcon = xwPos->second.connections;

        assert (args.num > 0);
        const auto cellIx = static_cast<std::size_t>(args.num) - 1;
        auto conn = std::find_if(xcon.begin(), xcon.end(),
            [cellIx](const Opm::data::Connection& c) -> bool
        {
            return c.index == cellIx;
        });

        if (conn == xcon.end()) {
            return retval;
        }

        // Connection found.  Invoke callback to calculate value.
        connOp(*conn, wname, retval);

        return retval;
    }

    template <class ConnOp>
    Opm::SummaryHelpers::SummaryQuantity
    connectionStaticQuantity(const Opm::SummaryHelpers::EvaluationArguments& args,
                             Opm::SummaryHelpers::SummaryQuantity            retval,
                             ConnOp&&                                        connOp)
    {
        if (args.schedule_wells.empty()) {
            return retval;
        }

        const auto& wname = args.schedule_wells.front();
        if (! args.sched.hasWell(wname, args.sim_step)) {
            return retval;
        }

        const auto& wcon = args.sched.getWell2(wname, args.sim_step)
            .getConnections();

        const auto cellID = static_cast<std::size_t>(args.num) - 1;

        auto connPos = std::find_if(wcon.begin(), wcon.end(),
            [&args, cellID](const Opm::Connection& conn)
        {
            return cellID == args.grid
                .getGlobalIndex(conn.getI(), conn.getJ(), conn.getK());
        });

        if (connPos == wcon.end()) {
            return retval;
        }

        // Connection found.  Invoke callback to calculate value.
        connOp(*connPos, wname, retval);

        return retval;
    }

    double
    efac(const std::vector<std::pair<std::string,double>>& eff_factors,
         const std::string&                                name)
    {
        auto it = std::find_if(eff_factors.begin(), eff_factors.end(),
            [&name](const std::pair<std::string, double> elem)
        {
            return elem.first == name;
        });

        return (it != eff_factors.end()) ? it->second : 1.0;
    }

    double polymerConcentraction(const Opm::Schedule& sched,
                                 const std::size_t    timeStep,
                                 const std::string&   wellname)
    {
        return sched.getWell2(wellname, timeStep)
            .getPolymerProperties().m_polymerConcentration;
    }

    template <Opm::data::Rates::opt phase, bool polymer>
    double flowRate(const Opm::data::Rates&                         rates,
                    const Opm::SummaryHelpers::EvaluationArguments& args,
                    const std::string&                              wellname)
    {
        const auto concentration = polymer
            ? polymerConcentraction(args.sched, args.sim_step, wellname)
            : 1.0;

        return rates.get(phase, 0.0)
            * efac(args.eff_factors, wellname) * concentration;
    }

    namespace BaseOperations
    {
        namespace SH = Opm::SummaryHelpers;

        template <
            Opm::data::Rates::opt phase,
            bool injection = true,
            bool polymer = false
        >
        struct Rate
        {
        private:
            using unitType = typename Dynamic::Rate<phase, polymer>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                double sum = 0.0;

                for (const auto& wname : args.schedule_wells) {
                    auto xwPos = args.well_sol.find(wname);
                    if (xwPos == args.well_sol.end()) { continue; }

                    const auto v = flowRate<phase, polymer>
                        (xwPos->second.rates, args, wname);

                    if ((v > 0.0) == injection) {
                        sum += v;
                    }
                }

                if (! injection) { sum = -sum; }

                return { sum, unitOfMeasure(unitType{}) };
            }
        };

        template <
            Opm::data::Rates::opt phase,
            bool injection = true,
            bool polymer = false
        >
        struct ConnRate
        {
        private:
            using unitType = typename Dynamic::Rate<phase, polymer>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                return connectionResultQuantity(args,
                    { 0.0, unitOfMeasure(unitType{}) },
                    [&args](const Opm::data::Connection& xcon,
                            const std::string&           wname,
                            SH::SummaryQuantity&         retval) -> void
                {
                    const auto v =
                        flowRate<phase, polymer>(xcon.rates, args, wname);

                    if ((v > 0.0) == injection) {
                        retval.value = injection ? v : -v;
                    }
                });
            }
        };

        struct ConnTrans
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                return connectionStaticQuantity(args,
                    { 0.0, Opm::UnitSystem::measure::transmissibility },
                    [](const Opm::Connection& conn,
                       const std::string&  /* wname */,
                       SH::SummaryQuantity&   retval) -> void
                {
                    retval.value = conn.CF() * conn.wellPi();
                });
            }
        };

        template <Opm::data::Rates::opt phase, bool polymer = false>
        struct SegmentRate
        {
        private:
            using unitType = typename Dynamic::Rate<phase, polymer>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                const auto unit = unitOfMeasure(unitType{});
                const auto zero = SH::SummaryQuantity{ 0.0, unit };

                if (args.schedule_wells.empty()) {
                    return zero;
                }

                const auto& wname = args.schedule_wells.front();

                auto xwPos = args.well_sol.find(wname);
                if (xwPos == args.well_sol.end()) {
                    return zero;
                }

                const auto segNo = static_cast<std::size_t>(args.num);
                auto seg = xwPos->second.segments.find(segNo);
                if (seg == xwPos->second.segments.end()) {
                    return zero;
                }

                // Sign convention differs in Flow vs ECLIPSE
                const auto v = -flowRate<phase, polymer>
                    (seg->second.rates, args, wname);

                return { v, unit };
            }
        };

        struct SegmentPressure
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                const auto unit = Opm::UnitSystem::measure::pressure;
                const auto zero = SH::SummaryQuantity{ 0.0, unit };

                if (args.schedule_wells.empty()) { return zero; }

                const auto& wname = args.schedule_wells.front();

                auto xwPos = args.well_sol.find(wname);
                if (xwPos == args.well_sol.end()) { return zero; }

                // Like completion rate we need to look
                // up a connection with offset 0.
                const auto segNumber = static_cast<std::size_t>(args.num);

                const auto& rseg = xwPos->second.segments;
                auto seg = rseg.find(segNumber);
                if (seg == rseg.end()) { return zero; }

                return { seg->second.pressure, unit };
            }
        };

        struct BottomHolePressure
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                const auto unit = Opm::UnitSystem::measure::pressure;
                const auto zero = SH::SummaryQuantity { 0.0, unit };

                if (args.schedule_wells.empty()) { return zero; }

                const auto& wname = args.schedule_wells.front();

                auto xwel = args.well_sol.find(wname);
                if (xwel == args.well_sol.end()) { return zero; }

                return { xwel->second.bhp, unit };
            }
        };

        struct TubingHeadPressure
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                const auto unit = Opm::UnitSystem::measure::pressure;
                const auto zero = SH::SummaryQuantity { 0.0, unit };

                if (args.schedule_wells.empty()) { return zero; }

                const auto& wname = args.schedule_wells.front();

                auto xwel = args.well_sol.find(wname);
                if (xwel == args.well_sol.end()) { return zero; }

                return { xwel->second.thp, unit };
            }
        };

        struct ObservedBottomHolePressure
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                const auto unit = Opm::UnitSystem::measure::pressure;
                const auto zero = SH::SummaryQuantity { 0.0, unit };

                if (args.schedule_wells.empty()) { return zero; }

                const auto& wname = args.schedule_wells.front();
                const auto& well  = args.sched
                    .getWell2(wname, args.sim_step);

                const auto obs_bhp = well.isProducer()
                    ? well.getProductionProperties().BHPH
                    : well.getInjectionProperties() .BHPH;

                return { obs_bhp, unit };
            }
        };

        struct ObservedTubingHeadPressure
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                const auto unit = Opm::UnitSystem::measure::pressure;
                const auto zero = SH::SummaryQuantity { 0.0, unit };

                if (args.schedule_wells.empty()) { return zero; }

                const auto& wname = args.schedule_wells.front();
                const auto& well  = args.sched
                    .getWell2(wname, args.sim_step);

                const auto obs_thp = well.isProducer()
                    ? well.getProductionProperties().THPH
                    : well.getInjectionProperties() .THPH;

                return { obs_thp, unit };
            }
        };

        template <Opm::Phase phase>
        struct ObservedProductionRate
        {
        private:
            using unitType = typename Declared::Rate<phase>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                // For well data, looking up historical rates (both for
                // production and injection) before simulation actually
                // starts is impossible and nonsensical.  We therefore
                // default to writing zero (which is what ECLIPSE seems
                // to do as well).

                double sum = 0.0;
                for (const auto& wname : args.schedule_wells) {
                    sum += args.sched.getWell2(wname, args.sim_step)
                        .production_rate(args.st, phase)
                        * efac(args.eff_factors, wname);
                }

                return { sum, unitOfMeasure(unitType{}) };
            }
        };

        template <Opm::Phase phase>
        struct ObservedInjectionRate
        {
        private:
            using unitType = typename Declared::Rate<phase>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                // For well data, looking up historical rates (both for
                // production and injection) before simulation actually
                // starts is impossible and nonsensical.  We therefore
                // default to writing zero (which is what ECLIPSE seems
                // to do as well).

                double sum = 0.0;
                for (const auto& wname : args.schedule_wells) {
                    sum += args.sched.getWell2(wname, args.sim_step)
                        .injection_rate(args.st, phase)
                        * efac(args.eff_factors, wname);
                }

                return { sum, unitOfMeasure(unitType{}) };
            }
        };

        struct ResVRateTarget
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                double sum = 0.0;

                for (const auto& wname : args.schedule_wells) {
                    const auto& pprod = args.sched
                        .getWell2(wname, args.sim_step)
                        .getProductionProperties();

                    if (pprod.predictionMode) {
                        sum += pprod.ResVRate.get<double>();
                    }
                }

                return { sum, Opm::UnitSystem::measure::rate };
            }
        };

        struct Duration
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                return { args.duration, Opm::UnitSystem::measure::time };
            }
        };

        template <Opm::data::Rates::opt phase, bool injection>
        struct RegionRate
        {
        private:
            using unitType = typename Dynamic::Rate<phase, false>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                double sum = 0.0;

                for (const auto& wcon : args.region_cache.connections(args.num)) {
                    const auto rate = args.well_sol
                        .get(wcon.first, wcon.second, phase)
                        * efac(args.eff_factors, wcon.first);

                    if ((rate > 0.0) == injection) {
                        sum += rate;
                    }
                }

                if (! injection) { sum = -sum; }

                return { sum, unitOfMeasure(unitType{}) };
            }
        };

        template <Opm::data::Rates::opt phase, bool outputInjector>
        struct PotentialRate
        {
        private:
            static_assert((phase == Opm::data::Rates::opt::well_potential_gas) ||
                          (phase == Opm::data::Rates::opt::well_potential_oil) ||
                          (phase == Opm::data::Rates::opt::well_potential_water),
                          "Phase must be well_potential_{gas,oil,water}");

            using unitType = typename Dynamic::Rate<phase, false>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                double sum = 0.0;

                for (const auto& wname : args.schedule_wells) {
                    auto xwPos = args.well_sol.find(wname);
                    if (xwPos == args.well_sol.end()) {
                        continue;
                    }

                    const auto& well = args.sched
                        .getWell2(wname, args.sim_step);

                    if (( outputInjector && well.isInjector()) ||
                        (!outputInjector && well.isProducer()))
                    {
                        sum += xwPos->second.rates.get(phase, 0.0);
                    }
                }

                // Flow's potential rates are always non-negative.
                return { sum, unitOfMeasure(unitType{}) };
            }
        };

        template <Opm::data::Rates::opt phase>
        struct ProductivityIndex
        {
            using unitType = typename Dynamic::ProdIndex<phase>::unit;

        public:
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                const auto unit = unitOfMeasure(unitType{});
                const auto zero = SH::SummaryQuantity{ 0.0, unit };

                if (args.schedule_wells.empty()) {
                    return zero;
                }

                const auto& wname = args.schedule_wells.front();
                auto xwPos = args.well_sol.find(wname);
                if (xwPos == args.well_sol.end()) {
                    return zero;
                }

                return { xwPos->second.rates.get(phase, 0.0), unit };
            }
        };

        template <typename Predicate>
        struct WellCount
        {
            SH::SummaryQuantity
            operator()(const SH::EvaluationArguments& args) const
            {
                Predicate p{};

                auto n = 0;
                for (const auto& wname : args.schedule_wells) {
                    if (p(wname, args)) {
                        n += 1;
                    }
                }

                return {
                    static_cast<double>(n),
                    Opm::UnitSystem::measure::identity
                };
            }
        };

        template <bool injection>
        struct Flowing
        {
            bool operator()(const std::string&             wname,
                            const SH::EvaluationArguments& args) const
            {
                auto xwPos = args.well_sol.find(wname);
                if ((xwPos == args.well_sol.end()) ||
                    !xwPos->second.flowing())
                {
                    // No results or all rates zero for this well.
                    return false;
                }

                // Well is flowing, check if well type matches desired.
                return args.sched.getWell2(wname, args.sim_step)
                    .isInjector() == injection;
            }
        };

        template <bool injection>
        struct Total
        {
            bool operator()(const std::string&             wname,
                            const SH::EvaluationArguments& args) const
            {
                return args.sched.getWell2(wname, args.sim_step)
                    .isInjector() == injection;
            }
        };

        template <typename Left, typename Right>
        SH::Evaluator add(Left&& left, Right&& right)
        {
            return SH::Evaluator{ std::forward<Left> (left)  }
                +  SH::Evaluator{ std::forward<Right>(right) };
        }

        template <typename Left, typename Right>
        SH::Evaluator subtract(Left&& left, Right&& right)
        {
            return SH::Evaluator{ std::forward<Left> (left)  }
                -  SH::Evaluator{ std::forward<Right>(right) };
        }

        template <typename Left, typename Right>
        SH::Evaluator multiply(Left&& left, Right&& right)
        {
            return SH::Evaluator{ std::forward<Left> (left)  }
                *  SH::Evaluator{ std::forward<Right>(right) };
        }

        template <typename Left, typename Right>
        SH::Evaluator divide(Left&& left, Right&& right)
        {
            return SH::Evaluator{ std::forward<Left> (left)  }
                /  SH::Evaluator{ std::forward<Right>(right) };
        }

        template <typename RateVariable>
        Opm::SummaryHelpers::Evaluator cumulative(RateVariable r)
        {
            return multiply(std::move(r), Duration{});
        }
    } // namespace BaseOperations

    class EvaluatorTable
    {
    public:
        EvaluatorTable();

        Opm::SummaryHelpers::Evaluator
        get(const std::string& keyword) const;

        std::vector<std::string> supportedKeywords() const;

    private:
        using Table = std::unordered_map<
            std::string, Opm::SummaryHelpers::Evaluator
        >;
        using VT = Table::value_type;

        Table funcTable_{};

        void initWellSpecificParameters();
        void initGroupSpecificParameters();
        void initConnectionParameters();
        void initRegionParameters();
        void initSegmentParameters();
        void initFlowParameters(const char prefix);
    };

    EvaluatorTable::EvaluatorTable()
        : funcTable_{}
    {
        this->initFlowParameters('W');
        this->initWellSpecificParameters();

        this->initFlowParameters('G');
        this->initFlowParameters('F');
        this->initGroupSpecificParameters();

        this->initConnectionParameters();
        this->initRegionParameters();
        this->initSegmentParameters();
    }

    Opm::SummaryHelpers::Evaluator
    EvaluatorTable::get(const std::string& keyword) const
    {
        auto fpos = this->funcTable_.find(keyword);

        if (fpos == this->funcTable_.end()) {
            return {};
        }

        return fpos->second;
    }

    std::vector<std::string> EvaluatorTable::supportedKeywords() const
    {
        auto kw = std::vector<std::string>{};
        kw.reserve(this->funcTable_.size());

        for (const auto& function : this->funcTable_) {
            kw.push_back(function.first);
        }

        return kw;
    }

    void EvaluatorTable::initWellSpecificParameters()
    {
        using namespace BaseOperations;
        using r = Opm::data::Rates::opt;

        const auto PIW = ProductivityIndex<r::productivity_index_water>{};
        const auto PIO = ProductivityIndex<r::productivity_index_oil>{};

        this->funcTable_.insert({
            VT{ "WBHP" , BottomHolePressure{} },
            VT{ "WTHP" , TubingHeadPressure{} },
            VT{ "WBHPH", ObservedBottomHolePressure{} },
            VT{ "WTHPH", ObservedTubingHeadPressure{} },
            VT{ "WPIW" , PIW },
            VT{ "WPIO" , PIO },
            VT{ "WPIG" , ProductivityIndex<r::productivity_index_gas>{} },
            VT{ "WPIL" , add(PIW, PIO) },
        });
    }

    void EvaluatorTable::initGroupSpecificParameters()
    {
        using namespace BaseOperations;

        const auto inj  = true;
        const auto prod = !inj;

        const auto MWIN = WellCount<Flowing<inj>>{};
        const auto MWIT = WellCount<Total<inj>>{};

        const auto MWPR = WellCount<Flowing<prod>>{};
        const auto MWPT = WellCount<Total<prod>>{};

        this->funcTable_.insert({
            VT{ "GMWIN", MWIN },  VT{ "FMWIN", MWIN },
            VT{ "GMWIT", MWIT },  VT{ "FMWIT", MWIT },
            VT{ "GMWPR", MWPR },  VT{ "FMWPR", MWPR },
            VT{ "GMWPT", MWPT },  VT{ "FMWPT", MWPT },
        });
    }

    void EvaluatorTable::initConnectionParameters()
    {
        using namespace BaseOperations;
        using r = Opm::data::Rates::opt;

        const auto inj  = true;
        const auto prod = !inj;
        const auto poly = true;

        const auto CIR = ConnRate<r::wat    , inj, poly>{};
        const auto GIR = ConnRate<r::gas    , inj      >{};
        const auto OIR = ConnRate<r::oil    , inj      >{};
        const auto NIR = ConnRate<r::solvent, inj      >{};
        const auto WIR = ConnRate<r::wat    , inj      >{};

        const auto GPR = ConnRate<r::gas    , prod>{};
        const auto OPR = ConnRate<r::oil    , prod>{};
        const auto NPR = ConnRate<r::solvent, prod>{};
        const auto WPR = ConnRate<r::wat    , prod>{};

        this->funcTable_.insert({
            // ------------ Injection --------------------
            VT{ "CCIR", CIR },  VT{ "CCIT", cumulative(CIR) },
            VT{ "CGIR", GIR },  VT{ "CGIT", cumulative(GIR) },
            VT{ "COIR", OIR },  VT{ "COIT", cumulative(OIR) },
            VT{ "CNIR", NIR },  VT{ "CNIT", cumulative(NIR) },
            VT{ "CWIR", WIR },  VT{ "CWIT", cumulative(WIR) },

            // ------------ Production -------------------
            VT{ "CGPR", GPR },  VT{ "CGPT", cumulative(GPR) },
            VT{ "COPR", OPR },  VT{ "COPT", cumulative(OPR) },
            VT{ "CNPR", NPR },  VT{ "CNPT", cumulative(NPR) },
            VT{ "CWPR", WPR },  VT{ "CWPT", cumulative(WPR) },

            // ------------ Free flow rate ---------------
            VT{ "CNFR", subtract(NPR, NIR) }, // Prod <=> positive

            // ------------ Ancillary quantities ---------
            VT{ "CTFAC", ConnTrans{} },
        });
    }

    void EvaluatorTable::initRegionParameters()
    {
        using namespace BaseOperations;
        using r = Opm::data::Rates::opt;

        const auto inj  = true;
        const auto prod = !inj;

        const auto OIR = RegionRate<r::oil, inj>{};
        const auto GIR = RegionRate<r::gas, inj>{};
        const auto WIR = RegionRate<r::wat, inj>{};

        const auto OPR = RegionRate<r::oil, prod>{};
        const auto GPR = RegionRate<r::gas, prod>{};
        const auto WPR = RegionRate<r::wat, prod>{};

        this->funcTable_.insert({
            // ------------ Injection --------------------
            VT{ "RWIR", WIR },  VT{ "RWIT", cumulative(WIR) },
            VT{ "ROIR", OIR },  VT{ "ROIT", cumulative(OIR) },
            VT{ "RGIR", GIR },  VT{ "RGIT", cumulative(GIR) },

            // ------------ Production --------------------
            VT{ "RWPR", WPR },  VT{ "RWPT", cumulative(WPR) },
            VT{ "ROPR", OPR },  VT{ "ROPT", cumulative(OPR) },
            VT{ "RGPR", GPR },  VT{ "RGPT", cumulative(GPR) },
        });
    }

    void EvaluatorTable::initSegmentParameters()
    {
        using namespace BaseOperations;
        using r = Opm::data::Rates::opt;

        this->funcTable_.insert({
            VT{ "SOFR", SegmentRate<r::oil>{} },
            VT{ "SGFR", SegmentRate<r::gas>{} },
            VT{ "SWFR", SegmentRate<r::wat>{} },
            VT{ "SPR" , SegmentPressure{} },
        });
    }

    void EvaluatorTable::initFlowParameters(const char prefix)
    {
        using namespace BaseOperations;
        using r = Opm::data::Rates::opt;

        const auto X = [prefix](const std::string& name)
        {
            return prefix + name;
        };

        const auto inj  = true;
        const auto prod = !inj;
        const auto poly = true;

        const auto WIR = Rate<r::wat    , inj      >{};
        const auto OIR = Rate<r::oil    , inj      >{};
        const auto GIR = Rate<r::gas    , inj      >{};
        const auto LIR = add(WIR, OIR);
        const auto NIR = Rate<r::solvent, inj      >{};
        const auto CIR = Rate<r::wat    , inj, poly>{};

        const auto WVIR = Rate<r::reservoir_water, inj>{};
        const auto OVIR = Rate<r::reservoir_oil  , inj>{};
        const auto GVIR = Rate<r::reservoir_gas  , inj>{};

        const auto VIR = add(add(OVIR, GVIR), WVIR);

        const auto WIRH = ObservedInjectionRate<Opm::Phase::WATER>{};
        const auto OIRH = ObservedInjectionRate<Opm::Phase::OIL>{};
        const auto GIRH = ObservedInjectionRate<Opm::Phase::GAS>{};

        const auto WPI = PotentialRate<r::well_potential_water, inj>{};
        const auto OPI = PotentialRate<r::well_potential_oil, inj>{};
        const auto GPI = PotentialRate<r::well_potential_gas, inj>{};

        const auto WPR  = Rate<r::wat    , prod>{};
        const auto OPR  = Rate<r::oil    , prod>{};
        const auto GPR  = Rate<r::gas    , prod>{};
        const auto NPR  = Rate<r::solvent, prod>{};

        const auto WVPR = Rate<r::reservoir_water, prod>{};
        const auto OVPR = Rate<r::reservoir_oil  , prod>{};
        const auto GVPR = Rate<r::reservoir_gas  , prod>{};

        const auto WPRH = ObservedProductionRate<Opm::Phase::WATER>{};
        const auto OPRH = ObservedProductionRate<Opm::Phase::OIL>{};
        const auto GPRH = ObservedProductionRate<Opm::Phase::GAS>{};
        const auto LPRH = add(WPRH, OPRH);

        const auto WPP  = PotentialRate<r::well_potential_water, prod>{};
        const auto OPP  = PotentialRate<r::well_potential_oil, prod>{};
        const auto GPP  = PotentialRate<r::well_potential_gas, prod>{};

        const auto LPR  = add(WPR, OPR);
        const auto VPR  = add(add(OVPR, GVPR), WVPR);

        const auto GPRS = Rate<r::dissolved_gas, prod>{};
        const auto OPRS = Rate<r::vaporized_oil, prod>{};
        const auto GPRF = subtract(GPR, GPRS);
        const auto OPRF = subtract(OPR, OPRS);

        const auto WCT = divide(WPR, LPR);
        const auto GOR = divide(GPR, OPR);
        const auto GLR = divide(GPR, LPR);

        const auto WCTH = divide(WPRH, LPRH);
        const auto GORH = divide(GPRH, OPRH);
        const auto GLRH = divide(GPRH, LPRH);

        this->funcTable_.insert({
            // ------------ Injection --------------------
            VT{ X("WIR") , WIR  },  VT{ X("WIT") , cumulative(WIR)  },
            VT{ X("OIR") , OIR  },  VT{ X("OIT") , cumulative(OIR)  },
            VT{ X("GIR") , GIR  },  VT{ X("GIT") , cumulative(GIR)  },
            VT{ X("LIR") , LIR  },  VT{ X("LIT") , cumulative(LIR)  },
            VT{ X("NIR") , NIR  },  VT{ X("NIT") , cumulative(NIR)  },
            VT{ X("CIR") , CIR  },  VT{ X("CIT") , cumulative(CIR)  },
            VT{ X("VIR") , VIR  },  VT{ X("VIT") , cumulative(VIR)  },
            VT{ X("WIRH"), WIRH },  VT{ X("WITH"), cumulative(WIRH) },
            VT{ X("OIRH"), OIRH },  VT{ X("OITH"), cumulative(OIRH) },
            VT{ X("GIRH"), GIRH },  VT{ X("GITH"), cumulative(GIRH) },
            VT{ X("WVIR"), WVIR },  VT{ X("WVIT"), cumulative(WVIR) },
            VT{ X("OVIR"), OVIR },  VT{ X("OVIT"), cumulative(OVIR) },
            VT{ X("GVIR"), GVIR },  VT{ X("GVIT"), cumulative(GVIR) },
            VT{ X("WPI") , WPI  },
            VT{ X("OPI") , OPI  },
            VT{ X("GPI") , GPI  },

            // ------------ Production --------------------
            VT{ X("WPR") , WPR  },  VT{ X("WPT") , cumulative(WPR)  },
            VT{ X("OPR") , OPR  },  VT{ X("OPT") , cumulative(OPR)  },
            VT{ X("GPR") , GPR  },  VT{ X("GPT") , cumulative(GPR)  },
            VT{ X("NPR") , NPR  },  VT{ X("NPT") , cumulative(NPR)  },
            VT{ X("LPR") , LPR  },  VT{ X("LPT") , cumulative(LPR)  },
            VT{ X("VPR") , VPR  },  VT{ X("VPT") , cumulative(VPR)  },
            VT{ X("WPRH"), WPRH },  VT{ X("WPTH"), cumulative(WPRH) },
            VT{ X("OPRH"), OPRH },  VT{ X("OPTH"), cumulative(OPRH) },
            VT{ X("GPRH"), GPRH },  VT{ X("GPTH"), cumulative(GPRH) },
            VT{ X("LPRH"), LPRH },  VT{ X("LPTH"), cumulative(LPRH) },
            VT{ X("WVPR"), WVPR },  VT{ X("WVPT"), cumulative(WVPR) },
            VT{ X("OVPR"), OVPR },  VT{ X("OVPT"), cumulative(OVPR) },
            VT{ X("GVPR"), GVPR },  VT{ X("GVPT"), cumulative(GVPR) },
            VT{ X("GPRS"), GPRS },  VT{ X("GPTS"), cumulative(GPRS) },
            VT{ X("OPRS"), OPRS },  VT{ X("OPTS"), cumulative(OPRS) },
            VT{ X("GPRF"), GPRF },  VT{ X("GPTF"), cumulative(GPRF) },
            VT{ X("OPRF"), OPRF },  VT{ X("OPTF"), cumulative(OPRF) },
            VT{ X("WPP") , WPP  },
            VT{ X("OPP") , OPP  },
            VT{ X("GPP") , GPP  },
            VT{ X("VPRT"), ResVRateTarget{} },

            // ------------ Ratios --------------------
            VT{ X("WCT"), WCT },   VT{ X("WCTH"), WCTH },
            VT{ X("GOR"), GOR },   VT{ X("GORH"), GORH },
            VT{ X("GLR"), GLR },   VT{ X("GLRH"), GLRH },
        });
    }

    const EvaluatorTable& functionTable()
    {
        static const auto table = EvaluatorTable{};

        return table;
    }
} // namespace anonymous

std::unique_ptr<Opm::SummaryHelpers::Evaluator>
Opm::SummaryHelpers::
getParameterEvaluator(const std::string& parameterKeyword)
{
    auto func = functionTable().get(parameterKeyword);

    if (! func) {
        return {};
    }

    return std::unique_ptr<Evaluator> {
        new Evaluator(std::move(func))
    };
}

std::vector<std::string> Opm::SummaryHelpers::supportedKeywords()
{
    return functionTable().supportedKeywords();
}
