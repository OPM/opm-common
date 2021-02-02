/*
  Copyright 2018 Statoil ASA

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

#define BOOST_TEST_MODULE Aggregate_Group_Data
#include <opm/output/eclipse/AggregateGroupData.hpp>
#include <opm/output/eclipse/AggregateNetworkData.hpp>
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/AggregateWellData.hpp>

#include <opm/output/eclipse/VectorItems/intehead.hpp>
#include <opm/output/eclipse/VectorItems/group.hpp>
#include <opm/output/eclipse/VectorItems/well.hpp>
#include <opm/parser/eclipse/Python/Python.hpp>

#include <opm/output/data/Wells.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/io/eclipse/OutputStream.hpp>

#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
#include <cstddef>

namespace {


    Opm::Deck first_sim(std::string fname) {
        return Opm::Parser{}.parseFile(fname);
    }

    Opm::SummaryState sum_state()
    {
        auto state = Opm::SummaryState{std::chrono::system_clock::now()};

        state.update("WOPR:B-1H",   335.);
        state.update("WWPR:B-1H",   43.);
        state.update("WGPR:B-1H",   224578.);
        state.update("WGLIR:B-1H",  65987.);

        state.update("WOPR:B-2H",   235.);
        state.update("WWPR:B-2H",   33.);
        state.update("WGPR:B-2H",   124578.);
        state.update("WGLIR:B-2H",  55987.);

        state.update("WOPR:B-3H",   135.);
        state.update("WWPR:B-3H",   23.);
        state.update("WGPR:B-3H",   24578.);
        state.update("WGLIR:B-3H",  45987.);

        state.update("WOPR:C-1H",   435.);
        state.update("WWPR:C-1H",   53.);
        state.update("WGPR:C-1H",   324578.);
        state.update("WGLIR:C-1H",  75987.);

        state.update("WOPR:C-2H",   535.);
        state.update("WWPR:C-2H",   63.);
        state.update("WGPR:C-2H",   424578.);
        state.update("WGLIR:C-2H",  75987.);

        return state;
    }
}

struct SimulationCase
{
    explicit SimulationCase(const Opm::Deck& deck)
        : es   ( deck )
        , grid { deck }
        , python( std::make_shared<Opm::Python>() )
        , sched (deck, es, python )
    {}

    // Order requirement: 'es' must be declared/initialised before 'sched'.
    Opm::EclipseState es;
    Opm::EclipseGrid  grid;
    std::shared_ptr<Opm::Python> python;
    Opm::Schedule     sched;
};

// =====================================================================

BOOST_AUTO_TEST_SUITE(Aggregate_Network)


// test dimensions of multisegment data
BOOST_AUTO_TEST_CASE (Constructor)
{
    namespace VI = ::Opm::RestartIO::Helpers::VectorItems;
    const auto simCase = SimulationCase{first_sim("4_NETWORK_MODEL5_MSW_ALL.DATA")};

    Opm::EclipseState es = simCase.es;
    Opm::Runspec rspec   = es.runspec();
    Opm::SummaryState st = sum_state();
    Opm::Schedule     sched = simCase.sched;
    Opm::EclipseGrid  grid = simCase.grid;
    const auto& ioConfig = es.getIOConfig();
    const auto& units    = es.getUnits();


    // Report Step 1: 2008-10-10 --> 2011-01-20
    const auto rptStep = std::size_t{1};
    std::string outputDir = "./";
    std::string baseName = "4_NETWORK_MODEL5_MSW_ALL";
    Opm::EclIO::OutputStream::Restart rstFile {
        Opm::EclIO::OutputStream::ResultSet { outputDir, baseName },
        rptStep,
        Opm::EclIO::OutputStream::Formatted { ioConfig.getFMTOUT() },
        Opm::EclIO::OutputStream::Unified   { ioConfig.getUNIFOUT() }
    };

    double secs_elapsed = 3.1536E07;
    const auto ih = Opm::RestartIO::Helpers::
        createInteHead(es, grid, sched, secs_elapsed,
                       rptStep, rptStep+1, rptStep);

    auto networkData = Opm::RestartIO::Helpers::AggregateNetworkData(ih);
    networkData.captureDeclaredNetworkData(es, sched, units, rptStep, st, ih);

    BOOST_CHECK_EQUAL(static_cast<int>(networkData.getINode().size()), ih[VI::NINODE] * ih[VI::NODMAX]);
    BOOST_CHECK_EQUAL(static_cast<int>(networkData.getIBran().size()), ih[VI::NIBRAN] * ih[VI::NBRMAX]);
    BOOST_CHECK_EQUAL(static_cast<int>(networkData.getINobr().size()), ih[VI::NINOBR]);
    BOOST_CHECK_EQUAL(static_cast<int>(networkData.getZNode().size()), ih[VI::NZNODE] * ih[VI::NODMAX]);
    BOOST_CHECK_EQUAL(static_cast<int>(networkData.getRNode().size()), ih[VI::NRNODE] * ih[VI::NODMAX]);
    BOOST_CHECK_EQUAL(static_cast<int>(networkData.getRBran().size()), ih[VI::NRBRAN] * ih[VI::NODMAX]);
}

#if 0
BOOST_AUTO_TEST_CASE (Declared_Group_Data)
{
    const auto simCase = SimulationCase{first_sim()};

    // Report Step 1: 2115-01-01 --> 2015-01-05
    const auto rptStep = std::size_t{1};

    const auto ih = MockIH {
        static_cast<int>(simCase.sched.getWells(rptStep).size())
    };

    BOOST_CHECK_EQUAL(ih.nwells, MockIH::Sz{4});

    const auto smry = sim_state();
    const auto& units    = simCase.es.getUnits();
    auto agrpd = Opm::RestartIO::Helpers::AggregateGroupData{ih.value};
    agrpd.captureDeclaredGroupData(simCase.sched, units, rptStep, smry,
            ih.value);

    // IGRP (PROD)
    {
        auto start = 0*ih.nigrpz;

        const auto& iGrp = agrpd.getIGroup();
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  2); // Group GRP1 - Child group number one - equal to group WGRP1 (no 2)
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  3); // Group GRP1 - Child group number two - equal to group WGRP2 (no 3)
        BOOST_CHECK_EQUAL(iGrp[start + 4] ,  2); // Group GRP1 - No of child groups
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group GRP1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  1); // Group GRP1 - Group level (FIELD level is 0)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  5); // Group GRP1 - index of parent group (= 0 for FIELD)

        start = 1*ih.nigrpz;
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group WGRP1 - Child well number one - equal to well PROD1 (no 1)
        BOOST_CHECK_EQUAL(iGrp[start + 1] ,  3); // Group WGRP1 - Child well number two - equal to well WINJ1 (no 3)
        BOOST_CHECK_EQUAL(iGrp[start + 4] ,  2); // Group WGRP1 - No of child wells
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  0); // Group WGRP1 - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  2); // Group WGRP1 - Group level (FIELD level is 0)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 28] ,  1); // Group GRP1 - index of parent group (= 0 for FIELD)

        start = (ih.ngmaxz-1)*ih.nigrpz;
        BOOST_CHECK_EQUAL(iGrp[start + 0] ,  1); // Group FIELD - Child group number one - equal to group GRP1
        BOOST_CHECK_EQUAL(iGrp[start + 4] ,  1); // Group FIELD - No of child groups
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 26] ,  1); // Group FIELD - Group type (well group = 0, node group = 1)
        BOOST_CHECK_EQUAL(iGrp[start + ih.nwgmax + 27] ,  0); // Group FIELD - Group level (FIELD level is 0)
    }


    // XGRP (PROD)
    {
        using Ix = ::Opm::RestartIO::Helpers::VectorItems::XGroup::index;
        auto start = 0*ih.nxgrpz;

        const auto& xGrp = agrpd.getXGroup();
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  235.); // Group GRP1 - GOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  239.); // Group GRP1 - GWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  100237.); // Group GRP1 - GGPR

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilPrGuideRate], 345.6, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::OilPrGuideRate],
                          xGrp[start + Ix::OilPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatPrGuideRate], 456.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatPrGuideRate],
                          xGrp[start + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasPrGuideRate], 567.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::GasPrGuideRate],
                          xGrp[start + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::VoidPrGuideRate], 678.9, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::VoidPrGuideRate],
                          xGrp[start + Ix::VoidPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilInjGuideRate], 0.123, 1.0e-10);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatInjGuideRate], 1234.5, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatInjGuideRate],
                          xGrp[start + Ix::WatInjGuideRate_2]);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasInjGuideRate], 2345.6, 1.0e-10);

        start = 1*ih.nxgrpz;
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  23.); // Group WGRP1 - GOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  29.); // Group WGRP1 - GWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  50237.); // Group WGRP1 - GGPR

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilPrGuideRate], 456.7, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::OilPrGuideRate],
                          xGrp[start + Ix::OilPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatPrGuideRate], 567.8, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatPrGuideRate],
                          xGrp[start + Ix::WatPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasPrGuideRate], 678.9, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::GasPrGuideRate],
                          xGrp[start + Ix::GasPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::VoidPrGuideRate], 789.1, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::VoidPrGuideRate],
                          xGrp[start + Ix::VoidPrGuideRate_2]);

        BOOST_CHECK_CLOSE(xGrp[start + Ix::OilInjGuideRate], 1.23, 1.0e-10);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::WatInjGuideRate], 2345.6, 1.0e-10);
        BOOST_CHECK_EQUAL(xGrp[start + Ix::WatInjGuideRate],
                          xGrp[start + Ix::WatInjGuideRate_2]);
        BOOST_CHECK_CLOSE(xGrp[start + Ix::GasInjGuideRate], 3456.7, 1.0e-10);

        start = 2*ih.nxgrpz;
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  43.); // Group WGRP2 - GOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  59.); // Group WGRP2 - GWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  70237.); // Group WGRP2 - GGPR


        start = (ih.ngmaxz-1)*ih.nxgrpz;
        BOOST_CHECK_EQUAL(xGrp[start + 0] ,  3456.); // Group FIELD - FOPR
        BOOST_CHECK_EQUAL(xGrp[start + 1] ,  5678.); // Group FIELD - FWPR
        BOOST_CHECK_EQUAL(xGrp[start + 2] ,  2003456.); // Group FIELD - FGPR
    }

        // ZGRP (PROD)
    {
        auto start = 0*ih.nzgrpz;

        const auto& zGrp = agrpd.getZGroup();
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "GRP1    "); // Group GRP1 - GOPR

        start = 1*ih.nzgrpz;
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "WGRP1   "); // Group WGRP1 - GOPR

        start = 2*ih.nzgrpz;
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "WGRP2   "); // Group WGRP2 - GOPR

        start = (ih.ngmaxz-1)*ih.nzgrpz;
        BOOST_CHECK_EQUAL(zGrp[start + 0].c_str() ,  "FIELD   "); // Group FIELD - FOPR
    }

#endif
//}


BOOST_AUTO_TEST_SUITE_END()
