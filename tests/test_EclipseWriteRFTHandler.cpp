/*
  Copyright 2015 Statoil ASA.

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
#include "config.h"

#if HAVE_DYNAMIC_BOOST_TEST
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_MODULE EclipseRFTWriter
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/EclipseWriteRFTHandler.hpp>
#include <opm/output/eclipse/EclipseWriter.hpp>
#include <opm/output/Wells.hpp>
#include <opm/core/grid/GridManager.hpp>
#include <opm/core/grid/GridHelpers.hpp>
#include <opm/core/simulator/BlackoilState.hpp>
#include <opm/core/simulator/WellState.hpp>
#include <opm/core/simulator/SimulatorTimer.hpp>
#include <opm/core/props/phaseUsageFromDeck.hpp>
#include <opm/core/utility/parameters/ParameterGroup.hpp>
#include <opm/core/utility/Compat.hpp>

#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>

#include <ert/ecl/ecl_rft_file.h>
#include <ert/util/test_work_area.h>
#include <ert/util/util.h>

#include <vector>

namespace {

void verifyRFTFile(const std::string& rft_filename) {

    ecl_rft_file_type * new_rft_file = ecl_rft_file_alloc(rft_filename.c_str());
    std::shared_ptr<ecl_rft_file_type> rft_file;
    rft_file.reset(new_rft_file, ecl_rft_file_free);

    //Get RFT node for well/time OP_1/10 OKT 2008
    time_t recording_time = util_make_datetime(0, 0, 0, 10, 10, 2008);
    ecl_rft_node_type * ecl_rft_node = ecl_rft_file_get_well_time_rft(rft_file.get() , "OP_1" , recording_time);
    BOOST_CHECK(ecl_rft_node_is_RFT(ecl_rft_node));

    //Verify RFT data for completions (ijk) 9 9 1, 9 9 2 and 9 9 3 for OP_1
    const ecl_rft_cell_type * ecl_rft_cell1 = ecl_rft_node_lookup_ijk(ecl_rft_node, 8, 8, 0);
    const ecl_rft_cell_type * ecl_rft_cell2 = ecl_rft_node_lookup_ijk(ecl_rft_node, 8, 8, 1);
    const ecl_rft_cell_type * ecl_rft_cell3 = ecl_rft_node_lookup_ijk(ecl_rft_node, 8, 8, 2);

    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell1), 210088*0.00001, 0.00001);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell2), 210188*0.00001, 0.00001);
    BOOST_CHECK_CLOSE(ecl_rft_cell_get_pressure(ecl_rft_cell3), 210288*0.00001, 0.00001);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_sgas(ecl_rft_cell1), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_sgas(ecl_rft_cell2), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_sgas(ecl_rft_cell3), 0.0);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_swat(ecl_rft_cell1), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_swat(ecl_rft_cell2), 0.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_swat(ecl_rft_cell3), 0.0);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_soil(ecl_rft_cell1), 1.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_soil(ecl_rft_cell2), 1.0);
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_soil(ecl_rft_cell3), 1.0);

    BOOST_CHECK_EQUAL(ecl_rft_cell_get_depth(ecl_rft_cell1), (0.250 + (0.250/2)));
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_depth(ecl_rft_cell2), (2*0.250 + (0.250/2)));
    BOOST_CHECK_EQUAL(ecl_rft_cell_get_depth(ecl_rft_cell3), (3*0.250 + (0.250/2)));
}




Opm::DeckConstPtr createDeck(const std::string& eclipse_data_filename) {
    Opm::ParserPtr parser = std::make_shared<Opm::Parser>();
    Opm::DeckConstPtr deck = parser->parseFile(eclipse_data_filename , Opm::ParseContext());
    return deck;
}


std::shared_ptr<Opm::WellState> createWellState(std::shared_ptr<Opm::BlackoilState> blackoilState)
{
    std::shared_ptr<Opm::WellState> wellState = std::make_shared<Opm::WellState>();
    wellState->init(0, *blackoilState);
    return wellState;
}



std::shared_ptr<Opm::BlackoilState> createBlackoilState(int timeStepIdx, std::shared_ptr<Opm::GridManager> ourFineGridManagerPtr)
{
    const UnstructuredGrid &ug_grid = *ourFineGridManagerPtr->c_grid();

    std::shared_ptr<Opm::BlackoilState> blackoilState = std::make_shared<Opm::BlackoilState>(Opm::UgGridHelpers::numCells( ug_grid ) , Opm::UgGridHelpers::numFaces( ug_grid ), 3);
    size_t numCells = Opm::UgGridHelpers::numCells( ug_grid );

    auto &pressure = blackoilState->pressure();
    for (size_t cellIdx = 0; cellIdx < numCells; ++cellIdx) {
        pressure[cellIdx] = timeStepIdx*1e5 + 1e4 + cellIdx;
    }
    return blackoilState;
}



std::shared_ptr<Opm::EclipseWriter> createEclipseWriter(std::shared_ptr<const Opm::Deck> deck,
                                                        std::shared_ptr<Opm::EclipseState> eclipseState,
                                                        std::shared_ptr<Opm::GridManager> ourFineGridManagerPtr,
                                                        const int * compressedToCartesianCellIdx)
{

    const UnstructuredGrid &ourFinerUnstructuredGrid = *ourFineGridManagerPtr->c_grid();

    std::shared_ptr<Opm::EclipseWriter> eclipseWriter = std::make_shared<Opm::EclipseWriter>(eclipseState,
                                                                                             ourFinerUnstructuredGrid.number_of_cells,
                                                                                             compressedToCartesianCellIdx);

    return eclipseWriter;
}

}

BOOST_AUTO_TEST_CASE(test_EclipseWriterRFTHandler)
{

    std::string eclipse_data_filename    = "testRFT.DATA";
    test_work_area_type * new_ptr = test_work_area_alloc("test_EclipseWriterRFTHandler");
    std::shared_ptr<test_work_area_type> test_area;
    test_area.reset(new_ptr, test_work_area_free);
    test_work_area_copy_file(test_area.get(), eclipse_data_filename.c_str());


    std::shared_ptr<const Opm::Deck>   deck         = createDeck(eclipse_data_filename);
    std::shared_ptr<Opm::EclipseState> eclipseState = std::make_shared<Opm::EclipseState>(deck , Opm::ParseContext());

    std::shared_ptr<Opm::SimulatorTimer> simulatorTimer = std::make_shared<Opm::SimulatorTimer>();
    simulatorTimer->init(eclipseState->getSchedule()->getTimeMap());

    std::shared_ptr<Opm::GridManager>  ourFineGridManagerPtr = std::make_shared<Opm::GridManager>(eclipseState->getInputGrid());
    const UnstructuredGrid &ourFinerUnstructuredGrid = *ourFineGridManagerPtr->c_grid();
    const int* compressedToCartesianCellIdx = Opm::UgGridHelpers::globalCell(ourFinerUnstructuredGrid);

    std::shared_ptr<Opm::EclipseWriter> eclipseWriter = createEclipseWriter(deck,
                                                                            eclipseState,
                                                                            ourFineGridManagerPtr,
                                                                            compressedToCartesianCellIdx);
    tm t = boost::posix_time::to_tm( simulatorTimer->startDateTime() );
    eclipseWriter->writeInit( simulatorTimer->currentPosixTime(), std::mktime( &t ) );


    for (; simulatorTimer->currentStepNum() < simulatorTimer->numSteps(); ++ (*simulatorTimer)) {
        std::shared_ptr<Opm::BlackoilState> blackoilState2 = createBlackoilState(simulatorTimer->currentStepNum(),ourFineGridManagerPtr);
        std::shared_ptr<Opm::WellState> wellState = createWellState(blackoilState2);
        Opm::data::Wells wells { {}, wellState->bhp(), wellState->perfPress(),
                                 wellState->perfRates(), wellState->temperature(),
                                 wellState->wellRates() };
        eclipseWriter->writeTimeStep( simulatorTimer->reportStepNum(),
                                      simulatorTimer->currentPosixTime(),
                                      simulatorTimer->simulationTimeElapsed(),
                                      sim2solution( *blackoilState2, phaseUsageFromDeck( eclipseState ) ),
                                      wells, false);
    }

    std::string cwd(test_work_area_get_cwd(test_area.get()));
    std::string rft_filename = cwd + "/TESTRFT.RFT";
    verifyRFTFile(rft_filename);


}



