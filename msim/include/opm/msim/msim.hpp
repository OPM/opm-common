#ifndef ISIM_MAIN_HPP
#define ISIM_MAIN_HPP

#include <chrono>
#include <functional>
#include <map>
#include <string>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Action/State.hpp>

#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/data/Groups.hpp>
#include <opm/input/eclipse/Deck/UDAValue.hpp>


namespace Opm {

class EclipseIO;
class ParseContext;
class Parser;
class Python;
class SummaryState;
class UDQState;
class WellTestState;

class msim {

public:
    using well_rate_function = double(const EclipseState&, const Schedule&, const SummaryState& st, const data::Solution&, size_t report_step, double seconds_elapsed);
    using solution_function = void(const EclipseState&, const Schedule&, data::Solution&, size_t report_step, double seconds_elapsed);

    msim(const EclipseState& state);

    Opm::UDAValue uda_val();

    void well_rate(const std::string& well, data::Rates::opt rate, std::function<well_rate_function> func);
    void solution(const std::string& field, std::function<solution_function> func);
    void run(Schedule& schedule, EclipseIO& io, bool report_only);
    void post_step(Schedule& schedule, SummaryState& st, data::Solution& sol, data::Wells& well_data, data::GroupAndNetworkValues& group_nwrk_data, size_t report_step, const time_point& sim_time);

    Action::State action_state;

private:

    void run_step(const Schedule& schedule, WellTestState& wtest_state, SummaryState& st, UDQState& udq_state, data::Solution& sol, data::Wells& well_data, data::GroupAndNetworkValues& group_nwrk_data, size_t report_step, EclipseIO& io) const;
    void run_step(const Schedule& schedule, WellTestState& wtest_state, SummaryState& st, UDQState& udq_state, data::Solution& sol, data::Wells& well_data, data::GroupAndNetworkValues& group_nwrk_data, size_t report_step, double dt, EclipseIO& io) const;
    void output(WellTestState& wtest_state, SummaryState& st, const UDQState& udq_state, size_t report_step, bool substep, double seconds_elapsed, const data::Solution& sol, const data::Wells& well_data, const data::GroupAndNetworkValues& group_data, EclipseIO& io) const;
    void simulate(const Schedule& schedule, const SummaryState& st, data::Solution& sol, data::Wells& well_data, data::GroupAndNetworkValues& group_nwrk_data, size_t report_step, double seconds_elapsed, double time_step) const;

    EclipseState state;
    std::map<std::string, std::map<data::Rates::opt, std::function<well_rate_function>>> well_rates;
    std::map<std::string, std::function<solution_function>> solutions;
};
}


#endif
