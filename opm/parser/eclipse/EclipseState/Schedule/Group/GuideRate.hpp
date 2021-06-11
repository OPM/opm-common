/*
  Copyright 2019, 2020 Equinor ASA.

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

#ifndef GUIDE_RATE_HPP
#define GUIDE_RATE_HPP

#include <cstddef>
#include <ctime>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <stddef.h>

#include <opm/parser/eclipse/EclipseState/Schedule/Group/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group/GuideRateModel.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>

namespace Opm {

class Schedule;
class GuideRate {

public:
// used for potentials and well rates
struct RateVector {
    RateVector () = default;
    RateVector (double orat, double grat, double wrat) :
        oil_rat(orat),
        gas_rat(grat),
        wat_rat(wrat)
    {}

    static RateVector rateVectorFromGuideRate(double guide_rate, GuideRateModel::Target target, const RateVector& rates);

    double eval(Well::GuideRateTarget target) const;
    double eval(Group::GuideRateProdTarget target) const;
    double eval(GuideRateModel::Target target) const;


    bool operator==(const RateVector& other) const {
        return (this->oil_rat == other.oil_rat) &&
                (this->gas_rat == other.gas_rat) &&
                (this->wat_rat == other.wat_rat);
    }
    double oil_rat;
    double gas_rat;
    double wat_rat;
};


private:

struct GuideRateValue {
    GuideRateValue() = default;
    GuideRateValue(double t, const RateVector& v, GuideRateModel::Target tg):
        sim_time(t),
        value(v),
        target(tg)
    {}

    bool operator==(const GuideRateValue& other) const {
        return (this->sim_time == other.sim_time &&
                this->value == other.value);
    }

    bool operator!=(const GuideRateValue& other) const {
        return !(*this == other);
    }

    double sim_time { std::numeric_limits<double>::lowest() };
    RateVector value { std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest() };
    GuideRateModel::Target target { GuideRateModel::Target::NONE };
};

struct GRValState {
    GuideRateValue curr{};
    GuideRateValue prev{};
};

public:
    GuideRate(const Schedule& schedule);
    void   compute(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot, const bool update_now=false);
    void compute(const std::string& wgname, const Phase& phase, size_t report_step, double guide_rate);
    double get(const std::string& well, Well::GuideRateTarget target, const RateVector& rates) const;
    double get(const std::string& group, Group::GuideRateProdTarget target, const RateVector& rates) const;
    double get(const std::string& name, GuideRateModel::Target model_target, const RateVector& rates) const;
    double get(const std::string& group, const Phase& phase) const;
    bool has(const std::string& name) const;
    bool has(const std::string& name, const Phase& phase) const;

    // prototyping for now, might make it private later
    bool timeToUpdate(const double sim_time, const double time_interval) const;

private:
    void well_compute(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot, const bool update_now=false);
    void group_compute(const std::string& wgname, size_t report_step, double sim_time, double oil_pot, double gas_pot, double wat_pot);
    RateVector eval_form(const GuideRateModel& model, double oil_pot, double gas_pot, double wat_pot) const;
    double eval_group_pot() const;
    double eval_group_resvinj() const;

    void assign_grvalue(const std::string& wgname, const GuideRateModel& model, GuideRateValue&& value);
    // double get_grvalue_result(const GRValState& gr) const;

    using GRValPtr = std::unique_ptr<GRValState>;

    typedef std::pair<Phase,std::string> pair;

    struct pair_hash
    {
        template <class T1, class T2>
        std::size_t operator() (const std::pair<T1, T2> &pair) const
        {
            return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
        }
    };

    std::unordered_map<std::string, GRValPtr> values;
    std::unordered_map<pair, double, pair_hash> injection_group_values;
    std::unordered_map<std::string, RateVector > potentials;
    const Schedule& schedule;
};

}

#endif
