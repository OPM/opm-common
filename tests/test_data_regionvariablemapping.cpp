/*
  Copyright (c) 2025 Equinor ASA

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

#define BOOST_TEST_MODULE data_RegionVariableMapping

#include <boost/test/unit_test.hpp>

#include <opm/output/data/RegionVariableMapping.hpp>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(Basic_Operations)

BOOST_AUTO_TEST_CASE(Empty_Mapping)
{
    using RegionSet = Opm::data::RegionVariableMapping::RegionSet;
    using Variable = Opm::data::RegionVariableMapping::Variable;

    auto m = Opm::data::RegionVariableMapping{};

    m.prepareRegistration();
    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numRegionSets(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.numVariables(), std::size_t{0});

    BOOST_CHECK_MESSAGE(m.regionSets().empty(),
                        "Region set collection must be empty");

    BOOST_CHECK_MESSAGE(m.variables().empty(),
                        "Variable collection must be empty");

    BOOST_CHECK_MESSAGE(! m.index(RegionSet { "hello" }).has_value(),
                        R"(There must be no index for region set "hello")");

    BOOST_CHECK_MESSAGE(! m.index(Variable { "v" }).has_value(),
                        R"(There must be no index for variable "v")");

    BOOST_CHECK_MESSAGE(! m.isCumulative(Variable { "v" }).has_value(),
                        R"(There must be no cumulative flag for variable "v")");
}

BOOST_AUTO_TEST_CASE(Unique_Region_Sets)
{
    using RegionSet = Opm::data::RegionVariableMapping::RegionSet;
    using namespace std::string_literals;

    auto m = Opm::data::RegionVariableMapping{};

    m.prepareRegistration();

    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "EQLNUM" });
    m.add(RegionSet { "FIPABC" });
    m.add(RegionSet { "FIPF00" });

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numRegionSets(), std::size_t{4});

    BOOST_CHECK_MESSAGE(! m.regionSets().empty(),
                        "Region set collection must not be empty");

    BOOST_CHECK_MESSAGE(! m.index(RegionSet { "hello" }).has_value(),
                        R"(There must be no index for region set "hello")");

    {
        const auto expect = std::array {
            "EQLNUM"s, "FIPABC"s, "FIPF00"s, "FIPNUM"s,
        };

        const auto& rset = m.regionSets();

        BOOST_CHECK_EQUAL_COLLECTIONS(rset  .begin(), rset  .end(),
                                      expect.begin(), expect.end());
    }

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"FIPNUM"}).has_value(),
                          R"(Region set "FIPNUM" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"EQLNUM"}).has_value(),
                          R"(Region set "EQLNUM" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"FIPABC"}).has_value(),
                          R"(Region set "FIPABC" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"FIPF00"}).has_value(),
                          R"(Region set "FIPF00" must be known to mapping)");

    BOOST_CHECK_EQUAL(m.index(RegionSet{"FIPNUM"}).value(), std::size_t{3});
    BOOST_CHECK_EQUAL(m.index(RegionSet{"EQLNUM"}).value(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.index(RegionSet{"FIPABC"}).value(), std::size_t{1});
    BOOST_CHECK_EQUAL(m.index(RegionSet{"FIPF00"}).value(), std::size_t{2});
}

BOOST_AUTO_TEST_CASE(Repeated_Region_Sets)
{
    using RegionSet = Opm::data::RegionVariableMapping::RegionSet;
    using namespace std::string_literals;

    auto m = Opm::data::RegionVariableMapping{};

    m.prepareRegistration();

    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "EQLNUM" });
    m.add(RegionSet { "FIPABC" });
    m.add(RegionSet { "FIPF00" });
    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "FIPNUM" });
    m.add(RegionSet { "FIPF00" });
    m.add(RegionSet { "FIPF00" });
    m.add(RegionSet { "PVTNUM" });

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numRegionSets(), std::size_t{5});

    BOOST_CHECK_MESSAGE(! m.regionSets().empty(),
                        "Region set collection must not be empty");

    {
        const auto expect = std::array {
            "EQLNUM"s, "FIPABC"s, "FIPF00"s, "FIPNUM"s, "PVTNUM"s,
        };

        const auto& rset = m.regionSets();

        BOOST_CHECK_EQUAL_COLLECTIONS(rset  .begin(), rset  .end(),
                                      expect.begin(), expect.end());
    }

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"FIPNUM"}).has_value(),
                          R"(Region set "FIPNUM" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"EQLNUM"}).has_value(),
                          R"(Region set "EQLNUM" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"FIPABC"}).has_value(),
                          R"(Region set "FIPABC" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"FIPF00"}).has_value(),
                          R"(Region set "FIPF00" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(RegionSet{"PVTNUM"}).has_value(),
                          R"(Region set "PVTNUM" must be known to mapping)");

    BOOST_CHECK_EQUAL(m.index(RegionSet{"FIPNUM"}).value(), std::size_t{3});
    BOOST_CHECK_EQUAL(m.index(RegionSet{"EQLNUM"}).value(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.index(RegionSet{"FIPABC"}).value(), std::size_t{1});
    BOOST_CHECK_EQUAL(m.index(RegionSet{"FIPF00"}).value(), std::size_t{2});
    BOOST_CHECK_EQUAL(m.index(RegionSet{"PVTNUM"}).value(), std::size_t{4});
}

BOOST_AUTO_TEST_CASE(Unique_Variables)
{
    using Variable = Opm::data::RegionVariableMapping::Variable;
    using VariableIdx = Opm::data::RegionVariableMapping::VariableIdx;

    using namespace std::string_literals;

    auto m = Opm::data::RegionVariableMapping{};

    m.prepareRegistration();

    m.add(Variable { "OPTW" }, /* is_cumulative = */ true);
    m.add(Variable { "OPR"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "SIP"  }, /* is_cumulative = */ false);

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numVariables(), std::size_t{4});

    BOOST_CHECK_MESSAGE(! m.variables().empty(),
                        "Variable collection must not be empty");

    BOOST_CHECK_MESSAGE(! m.index(Variable { "hello" }).has_value(),
                        R"(There must be no index for variable "hello")");

    {
        const auto expect = std::array {
            "GIP"s, "OPR"s, "OPTW"s, "SIP"s,
        };

        const auto& vars = m.variables();

        BOOST_CHECK_EQUAL_COLLECTIONS(vars  .begin(), vars  .end(),
                                      expect.begin(), expect.end());
    }

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"OPTW"}).has_value(),
                          R"(Variable "OPTW" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"OPR"}).has_value(),
                          R"(Variable "OPR" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"GIP"}).has_value(),
                          R"(Variable "GIP" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"SIP"}).has_value(),
                          R"(Variable "SIP" must be known to mapping)");

    BOOST_CHECK_EQUAL(m.index(Variable{"OPTW"}).value(), std::size_t{2});
    BOOST_CHECK_EQUAL(m.index(Variable{"OPR"}).value(), std::size_t{1});
    BOOST_CHECK_EQUAL(m.index(Variable{"GIP"}).value(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.index(Variable{"SIP"}).value(), std::size_t{3});

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"OPTW"}).has_value(),
                        R"(Variable "OPTW" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"OPR"}).has_value(),
                        R"(Variable "OPR" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"GIP"}).has_value(),
                        R"(Variable "GIP" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"SIP"}).has_value(),
                        R"(Variable "SIP" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(*m.isCumulative(Variable{"OPTW"}),
                        R"(Variable "OPTW" must be cumulative)");

    BOOST_CHECK_MESSAGE(! *m.isCumulative(Variable{"OPR"}),
                        R"(Variable "OPR" must not be cumulative)");

    BOOST_CHECK_MESSAGE(! *m.isCumulative(Variable{"GIP"}),
                        R"(Variable "GIP" must not be cumulative)");

    BOOST_CHECK_MESSAGE(! *m.isCumulative(Variable{"SIP"}),
                        R"(Variable "SIP" must not be cumulative)");

    BOOST_CHECK_MESSAGE(! m.isCumulative(VariableIdx{0}),
                        "Variable at index 0 must not be cumulative");

    BOOST_CHECK_MESSAGE(! m.isCumulative(VariableIdx{1}),
                        "Variable at index 1 must not be cumulative");

    BOOST_CHECK_MESSAGE(m.isCumulative(VariableIdx{2}),
                        "Variable at index 2 must be cumulative");

    BOOST_CHECK_MESSAGE(! m.isCumulative(VariableIdx{3}),
                        "Variable at index 3 must not be cumulative");
}

BOOST_AUTO_TEST_CASE(Repeated_Variables)
{
    using Variable = Opm::data::RegionVariableMapping::Variable;
    using VariableIdx = Opm::data::RegionVariableMapping::VariableIdx;

    using namespace std::string_literals;

    auto m = Opm::data::RegionVariableMapping{};

    m.prepareRegistration();

    m.add(Variable { "OPTW" }, /* is_cumulative = */ true);
    m.add(Variable { "OPR"  }, /* is_cumulative = */ false);
    m.add(Variable { "OPR"  }, /* is_cumulative = */ false);
    m.add(Variable { "OPR"  }, /* is_cumulative = */ false);
    m.add(Variable { "OPR"  }, /* is_cumulative = */ false);
    m.add(Variable { "OPR"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "SIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "OPTW" }, /* is_cumulative = */ true);
    m.add(Variable { "OPTW" }, /* is_cumulative = */ true);
    m.add(Variable { "OPTW" }, /* is_cumulative = */ true);
    m.add(Variable { "OPTW" }, /* is_cumulative = */ true);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);
    m.add(Variable { "GIP"  }, /* is_cumulative = */ false);

    m.commitStructure();

    BOOST_CHECK_EQUAL(m.numVariables(), std::size_t{4});

    BOOST_CHECK_MESSAGE(! m.variables().empty(),
                        "Variable collection must not be empty");

    BOOST_CHECK_MESSAGE(! m.index(Variable { "hello" }).has_value(),
                        R"(There must be no index for variable "hello")");

    {
        const auto expect = std::array {
            "GIP"s, "OPR"s, "OPTW"s, "SIP"s,
        };

        const auto& vars = m.variables();

        BOOST_CHECK_EQUAL_COLLECTIONS(vars  .begin(), vars  .end(),
                                      expect.begin(), expect.end());
    }

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"OPTW"}).has_value(),
                          R"(Variable "OPTW" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"OPR"}).has_value(),
                          R"(Variable "OPR" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"GIP"}).has_value(),
                          R"(Variable "GIP" must be known to mapping)");

    BOOST_REQUIRE_MESSAGE(m.index(Variable{"SIP"}).has_value(),
                          R"(Variable "SIP" must be known to mapping)");

    BOOST_CHECK_EQUAL(m.index(Variable{"OPTW"}).value(), std::size_t{2});
    BOOST_CHECK_EQUAL(m.index(Variable{"OPR"}).value(), std::size_t{1});
    BOOST_CHECK_EQUAL(m.index(Variable{"GIP"}).value(), std::size_t{0});
    BOOST_CHECK_EQUAL(m.index(Variable{"SIP"}).value(), std::size_t{3});

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"OPTW"}).has_value(),
                        R"(Variable "OPTW" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"OPR"}).has_value(),
                        R"(Variable "OPR" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"GIP"}).has_value(),
                        R"(Variable "GIP" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(m.isCumulative(Variable{"SIP"}).has_value(),
                        R"(Variable "SIP" must have cumulative flag)");

    BOOST_CHECK_MESSAGE(*m.isCumulative(Variable{"OPTW"}),
                        R"(Variable "OPTW" must be cumulative)");

    BOOST_CHECK_MESSAGE(! *m.isCumulative(Variable{"OPR"}),
                        R"(Variable "OPR" must not be cumulative)");

    BOOST_CHECK_MESSAGE(! *m.isCumulative(Variable{"GIP"}),
                        R"(Variable "GIP" must not be cumulative)");

    BOOST_CHECK_MESSAGE(! *m.isCumulative(Variable{"SIP"}),
                        R"(Variable "SIP" must not be cumulative)");

    BOOST_CHECK_MESSAGE(! m.isCumulative(VariableIdx{0}),
                        "Variable at index 0 must not be cumulative");

    BOOST_CHECK_MESSAGE(! m.isCumulative(VariableIdx{1}),
                        "Variable at index 1 must not be cumulative");

    BOOST_CHECK_MESSAGE(m.isCumulative(VariableIdx{2}),
                        "Variable at index 2 must be cumulative");

    BOOST_CHECK_MESSAGE(! m.isCumulative(VariableIdx{3}),
                        "Variable at index 3 must not be cumulative");
}

BOOST_AUTO_TEST_SUITE_END()     // Basic_Operations

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Erroneous_Accesses)

BOOST_AUTO_TEST_CASE(Add_After_Commit)
{
    using RegionSet = Opm::data::RegionVariableMapping::RegionSet;
    using Variable = Opm::data::RegionVariableMapping::Variable;

    auto m = Opm::data::RegionVariableMapping{};

    m.prepareRegistration();
    m.commitStructure();

    BOOST_CHECK_THROW(m.add(RegionSet{"hello"}), std::logic_error);
    BOOST_CHECK_THROW(m.add(Variable{"hello"}, true), std::logic_error);
}

BOOST_AUTO_TEST_CASE(Index_Before_Commit)
{
    using RegionSet = Opm::data::RegionVariableMapping::RegionSet;
    using Variable = Opm::data::RegionVariableMapping::Variable;

    auto m = Opm::data::RegionVariableMapping{};

    m.prepareRegistration();

    m.add(RegionSet{"rs1"});
    m.add(RegionSet{"rs2"});
    m.add(RegionSet{"rs17"});
    m.add(RegionSet{"rs29"});
    m.add(Variable{"v1"}, true);
    m.add(Variable{"v10"}, true);
    m.add(Variable{"v02"}, false);

    BOOST_CHECK_THROW(m.index(Variable{"v1"}), std::logic_error);
    BOOST_CHECK_THROW(m.index(RegionSet{"rs17"}), std::logic_error);
}

BOOST_AUTO_TEST_SUITE_END()     // Erroneous_Accesses
