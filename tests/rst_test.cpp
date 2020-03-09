/*
  Copyright 2020 Equinor ASA

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
#include <iostream>
#include <vector>
#include <unordered_map>


#include <iostream>

#include <opm/io/eclipse/rst/state.hpp>
#include <opm/io/eclipse/ERst.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ErrorGuard.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>

void initLogging() {
    std::shared_ptr<Opm::StreamLog> cout_log = std::make_shared<Opm::StreamLog>(std::cout, Opm::Log::DefaultMessageTypes);
    Opm::OpmLog::addBackend( "COUT" , cout_log);
}

/*
  This is a small test application which can be used to check that the Schedule
  object is correctly initialized from a restart file. The two commandline
  arguments should be .DATA files where the first one should be a normal case,
  and the second case should be configured for a restart. The actual restart
  file referred to in the second .DATA file must also be available; the restart
  time configured in the second .DATA file must be within the time range covered
  by the first .DATA file.
*/

int main(int , char ** argv) {
    initLogging();
    std::string case1 = argv[1];
    std::string case2 = argv[2];
    Opm::ParseContext parseContext;
    Opm::ErrorGuard errors;
    Opm::Parser parser;

    auto full_deck = parser.parseFile(case1);
    auto rst_deck = parser.parseFile(case2);
    Opm::EclipseState full_state(full_deck);
    Opm::EclipseState rst_state(rst_deck);

    const auto& init_config = rst_state.getInitConfig();
    int report_step = init_config.getRestartStep();
    const auto& rst_filename = rst_state.getIOConfig().getRestartFileName( init_config.getRestartRootName(), report_step, false );
    Opm::EclIO::ERst rst_file(rst_filename);

    const auto& rst = Opm::RestartIO::RstState::load(rst_file, report_step);
    Opm::Schedule sched(full_deck, full_state);
    Opm::Schedule rst_sched(rst_deck, rst_state, &rst);

    if (Opm::Schedule::cmp(sched, rst_sched, report_step) ) {
        std::cout << "Schedule objects were equal!" << std::endl;
        std::exit( EXIT_SUCCESS );
    } else {
        std::cout << "Differences were encountered between the Schedule objects" << std::endl;
        std::exit( EXIT_FAILURE );
    }
}
