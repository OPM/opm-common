#ifndef ISIM_MAIN_HPP
#define ISIM_MAIN_HPP

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
    msim(const std::string& deck_file);
    msim(const std::string& deck_file, const Parser& parser, const ParseContext& parse_context);

    void run();
private:

    void run_step(data::Solution& sol, data::Wells& well_data, size_t report_step, EclipseIO& io) const;
    void run_step(data::Solution& sol, data::Wells& well_data, size_t report_step, double dt, EclipseIO& io) const;
    void output(size_t report_step, bool substep, double seconds_elapsed, const data::Solution& sol, const data::Wells& well_data, EclipseIO& io) const;

    Deck deck;
    EclipseState state;
    Schedule schedule;
    SummaryConfig summary_config;
};
}


#endif
