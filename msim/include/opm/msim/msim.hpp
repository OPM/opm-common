#ifndef ISIM_MAIN_HPP
#define ISIM_MAIN_HPP

#include <functional>
#include <string>
#include <map>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>


namespace Opm {

class EclipseIO;


class msim {

public:
    using well_rate_function = double(const EclipseState&, const Schedule&, const data::Solution&, size_t report_step, double seconds_elapsed);
    using solution_function = void(const EclipseState&, const Schedule&, data::Solution&, size_t report_step, double seconds_elapsed);

    msim(const std::string& deck_file);
    msim(const std::string& deck_file, const Parser& parser, const ParseContext& parse_context);

    void well_rate(const std::string& well, data::Rates::opt rate, std::function<well_rate_function> func);
    void solution(const std::string& field, std::function<solution_function> func);
    void run();
private:

    void run_step(data::Solution& sol, data::Wells& well_data, size_t report_step, EclipseIO& io) const;
    void run_step(data::Solution& sol, data::Wells& well_data, size_t report_step, double dt, EclipseIO& io) const;
    void output(size_t report_step, bool substep, double seconds_elapsed, const data::Solution& sol, const data::Wells& well_data, EclipseIO& io) const;
    void simulate(data::Solution& sol, data::Wells& well_data, size_t report_step, double seconds_elapsed, double time_step) const;

    Deck deck;
    EclipseState state;
    Schedule schedule;
    SummaryConfig summary_config;

    std::map<std::string, std::map<data::Rates::opt, std::function<well_rate_function>>> well_rates;
    std::map<std::string, std::function<solution_function>> solutions;
};
}


#endif
