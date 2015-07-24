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



#define BOOST_TEST_MODULE IOConfigTests

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>


using namespace Opm;

const std::string& deckStr =  "RUNSPEC\n"
                              "\n"
                              "DIMENS\n"
                              " 10 10 10 /\n"
                              "GRID\n"
                              "GRIDFILE\n"
                              " 0 1 /\n"
                              "\n"
                              "START\n"
                              " 21 MAY 1981 /\n"
                              "\n"
                              "TSTEP\n"
                              " 1 2 3 4 5 /\n"
                              "DATES\n"
                              " 1 JAN 1982 /\n"
                              " 1 JAN 1982 13:55:44 /\n"
                              " 3 JAN 1982 14:56:45.123 /\n"
                              "/\n"
                              "TSTEP\n"
                              " 9 10 /\n"
                              "\n";


const std::string& deckStr3 =  "RUNSPEC\n"
                               "UNIFIN\n"
                               "UNIFOUT\n"
                               "FMTIN\n"
                               "FMTOUT\n"
                               "\n"
                               "DIMENS\n"
                               "10 10 10 /\n"
                               "GRID\n"
                               "INIT\n"
                               "NOGGF\n"
                               "\n";

const std::string& deckStr4 =  "RUNSPEC\n"
                               "\n"
                               "DIMENS\n"
                              " 10 10 10 /\n"
                               "GRID\n"
                               "GRIDFILE\n"
                               " 0 0 /\n"
                               "\n";




static DeckPtr createDeck(const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input, Opm::ParseMode());
}




BOOST_AUTO_TEST_CASE(IOConfigTest) {

    DeckPtr deck = createDeck(deckStr);
    std::shared_ptr<const EclipseGrid> grid = std::make_shared<const EclipseGrid>( 10 , 10 , 10 );
    IOConfigPtr ioConfigPtr;
    BOOST_CHECK_NO_THROW(ioConfigPtr = std::make_shared<IOConfig>());

    std::shared_ptr<const GRIDSection> gridSection = std::make_shared<const GRIDSection>(deck);
    std::shared_ptr<const RUNSPECSection> runspecSection = std::make_shared<const RUNSPECSection>(deck);
    ioConfigPtr->handleGridSection(gridSection);
    ioConfigPtr->handleRunspecSection(runspecSection);

    Schedule schedule(grid , deck, ioConfigPtr);

    //If no BASIC keyord has been handled, no restart files should be written
    TimeMapConstPtr timemap = schedule.getTimeMap();
    for (size_t ts = 0; ts < timemap->numTimesteps(); ++ts) {
        BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(ts));
    }

    /*BASIC=1, write restart file for every timestep*/
    size_t timestep = 3;
    int basic       = 1;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(),timestep, basic);
    for (size_t ts = 0; ts < timemap->numTimesteps(); ++ts) {
        if (ts < 3) {
            BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(ts));
        } else {
            BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(ts));
        }
    }

    //Add timestep 11, 12, 13, 14, 15, 16
    const TimeMap* const_tmap = timemap.get();
    TimeMap* writableTimemap = const_cast<TimeMap*>(const_tmap);
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));


    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(11));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(12));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(13));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(14));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(15));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(16));


    /* BASIC=3, restart files are created every nth report time, n=3 */
    timestep      = 11;
    basic         = 3;
    int frequency = 3;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(),timestep, basic, frequency);

    for (size_t ts = timestep ; ts < timemap->numTimesteps(); ++ts) {
        if (((ts-timestep) % frequency) == 0) {
            BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(ts));
        } else {
            BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(ts));
        }
    }

    //Add timestep 17, 18, 19, 20, 21, 22, 23, 24, 25, 26
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1983
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1984
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1985
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1986
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1987

    /* BASIC=4, restart file is written at the first report step of each year.
       Optionally, if the mnemonic FREQ is set >1 the restart is written only every n'th year*/
    timestep      = 17;
    basic         = 4;
    frequency     = 0;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(),timestep, basic, frequency);

    /*Expected results: Write timestep for timestep 17, 20, 22, 23, 26*/

    for (size_t ts = 17; ts <= 26; ++ts) {
        if ((17 == ts) || (20 == ts) || (22 == ts) || (23 == ts) || (26 == ts)) {
            BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(ts));
        } else {
            BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(ts));
        }
    }


    //Add timestep 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1988
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1989
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1990
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1991
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(8760)); //1992

    /* BASIC=4, FREQ = 2 restart file is written at the first report step of each year.
       Optionally, if the mnemonic FREQ is set >1 the restart is written only every n'th year*/
    timestep      = 27;
    basic         = 4;
    frequency     = 2;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(), timestep, basic, frequency);

    //Expected results: Write timestep for 27, 32 and 36 (27, 30, 32, 33, 36 with frequency 2)

    for (size_t ts = 27; ts <= 36; ++ts) {
        if ((27 == ts) || (32 == ts) || (36 == ts)) {
            BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(ts));
        } else {
            BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(ts));
        }
    }



    //Add timestep 37, 38, 39, 40, 41, 42, 43, 44, 45, 46
    writableTimemap->addTStep(boost::posix_time::hours(24));  //february
    writableTimemap->addTStep(boost::posix_time::hours(650)); //march
    writableTimemap->addTStep(boost::posix_time::hours(24));  //march
    writableTimemap->addTStep(boost::posix_time::hours(24));  //march
    writableTimemap->addTStep(boost::posix_time::hours(24));  //march
    writableTimemap->addTStep(boost::posix_time::hours(650)); //april
    writableTimemap->addTStep(boost::posix_time::hours(24));  //april
    writableTimemap->addTStep(boost::posix_time::hours(650)); //may
    writableTimemap->addTStep(boost::posix_time::hours(24));  //may
    writableTimemap->addTStep(boost::posix_time::hours(24));  //may

    /* BASIC=5, restart file is written at the first report step of each month.
       Optionally, if the mnemonic FREQ is set >1 the restart is written only every n'th month*/
    timestep      = 37;
    basic         = 5;
    frequency     = 2;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(), timestep, basic, frequency);

    //Expected results: Write timestep for timestep  38, 44 (38, 42, 44 with frequency 2)

    for (size_t ts = 37; ts <= 46; ++ts) {
        if ((38 == ts) || (44 == ts)) {
            BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(ts));
        } else {
            BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(ts));
        }
    }

    //Add timestep 47, 48, 49
    writableTimemap->addTStep(boost::posix_time::hours(750)); //june
    writableTimemap->addTStep(boost::posix_time::hours(750)); //july
    writableTimemap->addTStep(boost::posix_time::hours(750)); //august

    /* BASIC=0, no restart files are written*/
    timestep      = 47;
    basic         = 0;
    frequency     = 0;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(), timestep, basic, frequency);

    BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(47));
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(48));
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(49));


    /************************* Test RPTSCHED RESTART *************************/

    //Add timestep 50, 51
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));

    /* RPTSCHED RESTART=1, restart files are written*/
    timestep       = 50;
    size_t restart = 1;
    ioConfigPtr->handleRPTSCHEDRestart(schedule.getTimeMap(), timestep, restart);

    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(50));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(51));

    //Add timestep 52, 53
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));

    /* RPTSCHED RESTART=0, no restart files are written*/
    timestep      = 52;
    restart       = 0;
    ioConfigPtr->handleRPTSCHEDRestart(schedule.getTimeMap(), timestep, restart);

    BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(52));
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(53));

    //Add timestep 54, 55
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    timestep      = 54;
    basic         = 0;
    ioConfigPtr->handleRPTSCHEDRestart(schedule.getTimeMap(), timestep, restart);


    /* RPTSCHED RESTART IGNORED IF RPTRST BASIC > 2 */

    //Add timestep 56, 57, 58, 59
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    timestep      = 56;
    basic         = 3;
    frequency     = 1;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(), timestep, basic, frequency);

    timestep      = 58;
    restart       = 0;
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(timestep));

    /* RPTSCHED RESTART IGNORED IF RPTRST BASIC > 2 */
    ioConfigPtr->handleRPTSCHEDRestart(schedule.getTimeMap(), timestep, restart);
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(timestep));


    /* RPTSCHED RESTART NOT IGNORED IF RPTRST BASIC <= 2 */
    //Add timestep 60, 61
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    timestep      = 60;
    basic         = 1;
    frequency     = 0;
    ioConfigPtr->handleRPTRSTBasic(schedule.getTimeMap(), timestep, basic, frequency);

    timestep      = 61;
    restart       = 0;
    writableTimemap->addTStep(boost::posix_time::hours(24));
    writableTimemap->addTStep(boost::posix_time::hours(24));
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(timestep));

    ioConfigPtr->handleRPTSCHEDRestart(schedule.getTimeMap(), timestep, restart);
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(timestep));

    /*Override, interval = 2*/
    ioConfigPtr->overrideRestartWriteInterval(2);
    for (size_t tstep = 0; tstep <= 61; ++tstep) {
        if ((tstep % 2) == 0) {
            BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteRestartFile(tstep));
        } else {
            BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(tstep));
        }
    }

    /*Override, turn off RESTART write*/
    ioConfigPtr->overrideRestartWriteInterval(0);
    for (size_t tstep = 0; tstep <= 61; ++tstep) {
        BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteRestartFile(tstep));
    }

    /*If no GRIDFILE nor NOGGF keywords are specified, default output an EGRID file*/
    BOOST_CHECK_EQUAL(true, ioConfigPtr->getWriteEGRIDFile());

    /*If no INIT keyword is specified, verify no write of INIT file*/
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getWriteINITFile());

    /*If no UNIFIN keyword is specified, verify UNIFIN false (default is multiple) */
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getUNIFIN());

    /*If no UNIFOUT keyword is specified, verify UNIFOUT false (default is multiple) */
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getUNIFOUT());

    /*If no FMTIN keyword is specified, verify FMTIN false (default is unformatted) */
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getFMTIN());

    /*If no FMTOUT keyword is specified, verify FMTOUT false (default is unformatted) */
    BOOST_CHECK_EQUAL(false, ioConfigPtr->getFMTOUT());

    /*If NOGGF keyword is present, no EGRID file is written*/
    DeckPtr deck3 = createDeck(deckStr3);
    IOConfigPtr ioConfigPtr3;
    BOOST_CHECK_NO_THROW(ioConfigPtr3 = std::make_shared<IOConfig>());

    std::shared_ptr<const GRIDSection> gridSection3 = std::make_shared<const GRIDSection>(deck3);
    std::shared_ptr<const RUNSPECSection> runspecSection3 = std::make_shared<const RUNSPECSection>(deck3);
    ioConfigPtr3->handleGridSection(gridSection3);
    ioConfigPtr3->handleRunspecSection(runspecSection3);

    BOOST_CHECK_EQUAL(false, ioConfigPtr3->getWriteEGRIDFile());

    /*If INIT keyword is specified, verify write of INIT file*/
    BOOST_CHECK_EQUAL(true, ioConfigPtr3->getWriteINITFile());

    /*If UNIFOUT keyword is specified, verify unified write*/
    BOOST_CHECK_EQUAL(true, ioConfigPtr3->getUNIFOUT());

    /*If FMTOUT keyword is specified, verify formatted write*/
    BOOST_CHECK_EQUAL(true, ioConfigPtr3->getFMTOUT());

    /*If GRIDFILE 0 0 is specified, no EGRID file is written */
    DeckPtr deck4 = createDeck(deckStr4);
    IOConfigPtr ioConfigPtr4;
    BOOST_CHECK_NO_THROW(ioConfigPtr4 = std::make_shared<IOConfig>());

    std::shared_ptr<const GRIDSection> gridSection4 = std::make_shared<const GRIDSection>(deck4);
    std::shared_ptr<const RUNSPECSection> runspecSection4 = std::make_shared<const RUNSPECSection>(deck4);
    ioConfigPtr4->handleGridSection(gridSection4);
    ioConfigPtr4->handleRunspecSection(runspecSection4);

    BOOST_CHECK_EQUAL(false, ioConfigPtr4->getWriteEGRIDFile());
}



