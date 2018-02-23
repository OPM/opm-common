#include <opm/json/JsonObject.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>


class SunbeamState {
public:
    SunbeamState(bool file_input, const std::string& deck_input);
    SunbeamState(bool file_input, const std::string& deck_input, const Opm::ParseContext& context);
    SunbeamState(bool file_input, const std::string& deck_input, const Opm::ParseContext& context, const Opm::Parser& parser);

    const Opm::EclipseState& getEclipseState() const;
    const Opm::Deck& getDeck() const;
    const Opm::Schedule getSchedule() const;
    const Opm::SummaryConfig getSummmaryConfig() const;

private:
    Opm::Deck deck;
    Opm::EclipseState ecl_state;
    Opm::Schedule schedule;
    Opm::SummaryConfig summary_config;
};
