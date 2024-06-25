/*
  Copyright 2013 Statoil ASA.

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

#include <opm/input/eclipse/Schedule/ScheduleStatic.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/L.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/S.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

namespace {

double sumthin_summary_section(const Opm::SUMMARYSection& section) {
    const auto entries = section.getKeywordList<Opm::ParserKeywords::SUMTHIN>();

    // Care only about the last SUMTHIN entry in the SUMMARY
    // section if keyword is present here at all.
    return entries.empty()
        ? -1.0 // (<= 0.0)
        : entries.back()->getRecord(0).getItem(0).getSIDouble(0);
}

bool rptonly_summary_section(const Opm::SUMMARYSection& section) {
    auto rptonly = false;

    using On = Opm::ParserKeywords::RPTONLY;
    using Off = Opm::ParserKeywords::RPTONLYO;

    // Last on/off keyword entry "wins".
    for (const auto& keyword : section) {
        if (keyword.is<On>())
            rptonly = true;
        else if (keyword.is<Off>())
            rptonly = false;
    }

    return rptonly;
}

std::optional<Opm::OilVaporizationProperties>
vappars_solution_section(const Opm::SOLUTIONSection& section, const int numpvt) {
    if (section.hasKeyword("VAPPARS")) {
        const auto& record = section.getKeyword("VAPPARS").getRecord(0);
        Opm::OilVaporizationProperties ovp(numpvt);
        double vap1 = record.getItem("OIL_VAP_PROPENSITY").get< double >(0);
        double vap2 = record.getItem("OIL_DENSITY_PROPENSITY").get< double >(0);
        Opm::OilVaporizationProperties::updateVAPPARS(ovp, vap1, vap2);
        return ovp;
    }
    return std::nullopt;
}
}

namespace Opm {

ScheduleStatic::ScheduleStatic(std::shared_ptr<const Python> python_handle,
                               const ScheduleRestartInfo& restart_info,
                               const Deck& deck,
                               const Runspec& runspec,
                               const std::optional<int>& output_interval_,
                               const ParseContext& parseContext,
                               ErrorGuard& errors,
                               const bool slave_mode) :
    m_python_handle(python_handle),
    m_input_path(deck.getInputPath()),
    rst_info(restart_info),
    m_deck_message_limits( deck ),
    m_unit_system( deck.getActiveUnitSystem() ),
    m_runspec( runspec ),
    rst_config( SOLUTIONSection(deck), parseContext, errors ),
    output_interval(output_interval_),
    slave_mode(slave_mode),
    sumthin(sumthin_summary_section(SUMMARYSection{ deck })),
    rptonly(rptonly_summary_section(SUMMARYSection{ deck })),
    gaslift_opt_active(deck.hasKeyword<ParserKeywords::LIFTOPT>()),
    oilVap(vappars_solution_section(SOLUTIONSection{deck},
                                    runspec.tabdims().getNumPVTTables()))
{}

ScheduleStatic ScheduleStatic::serializationTestObject()
{
    auto python = std::make_shared<Python>(Python::Enable::OFF);
    ScheduleStatic st(python);
    st.m_deck_message_limits = MessageLimits::serializationTestObject();
    st.m_runspec = Runspec::serializationTestObject();
    st.m_unit_system = UnitSystem::newFIELD();
    st.m_input_path = "Some/funny/path";
    st.rst_config = RSTConfig::serializationTestObject();
    st.rst_info = ScheduleRestartInfo::serializationTestObject();
    return st;
}

bool ScheduleStatic::operator==(const ScheduleStatic& other) const
{
    return this->m_input_path == other.m_input_path &&
           this->m_deck_message_limits == other.m_deck_message_limits &&
           this->m_unit_system == other.m_unit_system &&
           this->rst_config == other.rst_config &&
           this->rst_info == other.rst_info &&
           this->gaslift_opt_active == other.gaslift_opt_active &&
           this->m_runspec == other.m_runspec;
}

}
