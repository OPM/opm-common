/*
  Copyright (c) 2026 Equinor ASA

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

#define BOOST_TEST_MODULE SummaryConfig_RegionVariableSupport

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/SummaryConfig/RegionVariableSupport.hpp>

#include <opm/output/data/RegionVariableMapping.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <fmt/format.h>

#include <memory>
#include <string_view>

namespace {
    Opm::SummaryConfig makeSummaryConfig(std::string_view regions, std::string_view summary)
    {
        const auto deck = Opm::Parser{}.parseString(fmt::format(R"(RUNSPEC
DIMENS
  5 1 2 /
OIL
GAS
WATER
METRIC
TABDIMS
/
EQLDIMS
/
GRID
DXV
  5*100 /
DYV
  1*100 /
DZV
  5 10 /
DEPTHZ
  12*2000.0 /
EQUALS
  PORO 0.25 /
  PERMX 100 /
  PERMY 100 /
  PERMZ 10 /
/
PROPS
DENSITY
  850.0 1024.0 1.0 /
REGIONS
{regions}
SOLUTION
EQUIL
  2010.0 256.25 2013.0 0.0 2000.0 0.0 /
SUMMARY
{summary}
SCHEDULE
WELSPECS
  'I' 'G' 1 1 2002.5 'WATER' /
  'P' 'G' 5 1 2010.0 'LIQ' /
/
COMPDAT
  'I' 1 1 1 1 'OPEN' 1* 1* 0.5 /
  'P' 5 1 2 2 'OPEN' 1* 1* 0.5 /
/
TSTEP
  1 2 3 4 5 10 20 5*30 /
END
)", fmt::arg("regions", regions), fmt::arg("summary", summary)));

        const auto es = Opm::EclipseState{deck};

        const auto sched = Opm::Schedule{deck, es};

        const auto parseCtx = Opm::ParseContext{};
        auto errors = Opm::ErrorGuard{};

        auto summaryConfig = Opm::SummaryConfig {
            deck, sched, es.fieldProps(), es.aquifer(), parseCtx, errors
        };

        errors.clear();

        return summaryConfig;
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Basic_Operations)

BOOST_AUTO_TEST_CASE(No_Oil_Efficiency_From_Wells)
{
    const auto sumCfg = makeSummaryConfig(R"(FIPNUM
  5*1 5*2 /
FIPABC
  5*2 5*1 /
)", R"(WOPR
/
FOPT
ROPT
/
ROPT_ABC
/
)");

    auto m = Opm::data::RegionVariableMapping{};
    m.prepareRegistration();

    Opm::populateRegVarMapping(sumCfg, m);

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numRegionSets(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.numVariables(), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Default_Regions)
{
    const auto sumCfg = makeSummaryConfig("", R"(ROEW
1 /
)");

    auto m = Opm::data::RegionVariableMapping{};
    m.prepareRegistration();

    Opm::populateRegVarMapping(sumCfg, m);

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(m.numVariables(), std::size_t{1});

    BOOST_REQUIRE_MESSAGE(m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPNUM"}).has_value(),
                          R"(Region set "FIPNUM" must be known to mapping)");
    BOOST_REQUIRE_MESSAGE(m.index(Opm::data::RegionVariableMapping::Variable{"ConnOPT"}).has_value(),
                          R"(Variable "ConnOPT" must be known to mapping)");

    BOOST_CHECK_EQUAL(m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPNUM"}).value(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.index(Opm::data::RegionVariableMapping::Variable{"ConnOPT"}).value(), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Custom_Regions)
{
    const auto sumCfg = makeSummaryConfig(R"(FIPNUM
  5*1 5*2 /
FIPABC
  5*2 5*1 /)", R"(ROEW_ABC
1 /)");

    auto m = Opm::data::RegionVariableMapping{};
    m.prepareRegistration();

    Opm::populateRegVarMapping(sumCfg, m);

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(m.numVariables(), std::size_t{1});

    BOOST_CHECK_MESSAGE(! m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPNUM"}).has_value(),
                        R"(Region set "FIPNUM" must NOT be known to mapping)");
    BOOST_REQUIRE_MESSAGE(m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPABC"}).has_value(),
                          R"(Region set "FIPABC" must be known to mapping)");
    BOOST_REQUIRE_MESSAGE(m.index(Opm::data::RegionVariableMapping::Variable{"ConnOPT"}).has_value(),
                          R"(Variable "ConnOPT" must be known to mapping)");

    BOOST_CHECK_EQUAL(m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPABC"}).value(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.index(Opm::data::RegionVariableMapping::Variable{"ConnOPT"}).value(), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Field_Level)
{
    const auto sumCfg = makeSummaryConfig(R"(FIPNUM
  5*1 5*2 /)", R"(FOEW
)");

    auto m = Opm::data::RegionVariableMapping{};
    m.prepareRegistration();

    Opm::populateRegVarMapping(sumCfg, m);

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(m.numVariables(), std::size_t{1});

    BOOST_REQUIRE_MESSAGE(m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPNUM"}).has_value(),
                          R"(Region set "FIPNUM" must be known to mapping)");
    BOOST_CHECK_MESSAGE(! m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPABC"}).has_value(),
                          R"(Region set "FIPABC" must NOT be known to mapping)");
    BOOST_REQUIRE_MESSAGE(m.index(Opm::data::RegionVariableMapping::Variable{"ConnOPT"}).has_value(),
                          R"(Variable "ConnOPT" must be known to mapping)");

    BOOST_CHECK_EQUAL(m.index(Opm::data::RegionVariableMapping::RegionSet{"FIPNUM"}).value(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.index(Opm::data::RegionVariableMapping::Variable{"ConnOPT"}).value(), std::size_t{0});
}

BOOST_AUTO_TEST_SUITE_END() // Basic_Operations
