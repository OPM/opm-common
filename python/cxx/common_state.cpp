#include "common_state.hpp"


SunbeamState::SunbeamState(bool file_input, const std::string& deck_input, const Opm::ParseContext& context, const Opm::Parser& parser)
    : deck(file_input
          ? parser.parseFile(deck_input, context, guard)
          : parser.parseString(deck_input, context, guard)),
      ecl_state(deck, context, guard),
      schedule(deck, ecl_state, context, guard),
      summary_config(deck, schedule, ecl_state.getTableManager(), context, guard)
{
    guard.clear();
}


SunbeamState::SunbeamState(bool file_input, const std::string& deck_input) :
    SunbeamState(file_input, deck_input, Opm::ParseContext(), Opm::Parser())
{}


SunbeamState::SunbeamState(bool file_input, const std::string& deck_input, const Opm::ParseContext& context) :
    SunbeamState(file_input, deck_input, context, Opm::Parser())
{}


const Opm::EclipseState& SunbeamState::getEclipseState() const {
    return this->ecl_state;
}

const Opm::Deck& SunbeamState::getDeck() const {
    return this->deck;
}

const Opm::Schedule SunbeamState::getSchedule() const {
    return this->schedule;
}

const Opm::SummaryConfig SunbeamState::getSummmaryConfig() const {
    return this->summary_config;
}
