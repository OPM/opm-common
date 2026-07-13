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

#define BOOST_TEST_MODULE data_RegionVariableValues

#include <boost/test/unit_test.hpp>

#include <opm/output/data/RegionVariableValues.hpp>

#include <opm/output/data/RegionsetVariableDescriptor.hpp>
#include <opm/output/data/RegionVariableView.hpp>

#include <optional>
#include <vector>

BOOST_AUTO_TEST_SUITE(Single_RegSet)

namespace {
    Opm::data::RegionsetVariableDescriptor basic_fipnum()
    {
        auto descr = Opm::data::RegionsetVariableDescriptor{};

        descr.prepareDescriptorSet();

        descr.addRegionSet(3);  // Supports regions 1, 2, 3.  Strictly
                                // speaking also region 0.

        descr.finaliseDescriptorSet();

        return descr;
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Single_Variable)

BOOST_AUTO_TEST_SUITE(Non_Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1), 1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 1, 1.0);

    regVars.addRegionValue(0, 0, 2, -0.5);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1), 5.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), 1.5, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -0.5);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1),  1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), -0.5, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3),  0.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -0.5);
    regVars.addRegionValue(0, 0, 2,  0.5);
    regVars.addRegionValue(0, 0, 2,  0.5);
    regVars.addRegionValue(0, 0, 2, -0.25);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1), 4.0 , 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), 0.25, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3), 0.0 , 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Non_Cumulative

// ------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1), 1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 1, 1.0);

    regVars.addRegionValue(0, 0, 2, -0.5);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1), 5.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), 1.5, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -0.5);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), 1.5, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -0.5);
    regVars.addRegionValue(0, 0, 2,  0.5);
    regVars.addRegionValue(0, 0, 2,  0.5);
    regVars.addRegionValue(0, 0, 2, -0.25);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 1), 5.0 , 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 2), 2.25, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(0, 3), 3.0 , 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Cumulative

BOOST_AUTO_TEST_SUITE_END()     // Single_Variable

// ========================================================================

BOOST_AUTO_TEST_SUITE(Multi_Variable)

BOOST_AUTO_TEST_SUITE(Non_Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1), 10.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 20.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 30.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 15.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 25.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 35.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 125.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 225.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 325.0, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -2.5);

    regVars.addRegionValue(2, 0, 3,   5.0);
    regVars.addRegionValue(2, 0, 3, - 5.0);
    regVars.addRegionValue(2, 0, 3, - 5.0);
    regVars.addRegionValue(2, 0, 3,  12.0);

    regVars.addRegionValue(3, 0, 1, -25.0);
    regVars.addRegionValue(3, 0, 2, -25.0);
    regVars.addRegionValue(3, 0, 3, -25.0);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1),  5.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), -0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3),  3.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1), 10.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 20.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 30.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 15.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 25.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 42.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 100.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 200.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 300.0, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -0.5);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(3, 0, 1, 1.7);
    regVars.addRegionValue(3, 0, 2, 2.9);
    regVars.addRegionValue(3, 0, 3, 3.14);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1),  1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), -0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3),  0.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1), 11.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 22.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 33.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 0.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 1.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 2.9, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 3.14, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 3,  17.29);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 2, - 0.5);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);

    regVars.addRegionValue(3, 0, 1, 1.7);
    regVars.addRegionValue(3, 0, 2, 2.9);
    regVars.addRegionValue(3, 0, 3, 3.0);
    regVars.addRegionValue(3, 0, 3, 0.14);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1),   5.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), - 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3),  17.29, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1), 33.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 66.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 99.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 0.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 1.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 2.9, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 3.14, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Non_Cumulative

// ------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1), 10.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 20.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 30.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 15.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 25.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 35.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 125.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 225.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 325.0, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -2.5);

    regVars.addRegionValue(2, 0, 3,   5.0);
    regVars.addRegionValue(2, 0, 3, - 5.0);
    regVars.addRegionValue(2, 0, 3, - 5.0);
    regVars.addRegionValue(2, 0, 3,  12.0);

    regVars.addRegionValue(3, 0, 1, -25.0);
    regVars.addRegionValue(3, 0, 2, -25.0);
    regVars.addRegionValue(3, 0, 3, -25.0);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1),  5.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), -0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3),  3.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1), 10.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 20.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 30.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 15.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 25.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 42.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 100.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 200.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 300.0, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,  1.0);
    regVars.addRegionValue(0, 0, 2, -0.5);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(3, 0, 1, 1.7);
    regVars.addRegionValue(3, 0, 2, 2.9);
    regVars.addRegionValue(3, 0, 3, 3.14);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1), 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 1.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 3.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1), 21.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 42.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 63.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 15.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 25.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 35.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 126.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 227.9, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 328.14, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 3,  17.29);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 2, - 0.5);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);

    regVars.addRegionValue(3, 0, 1, 1.7);
    regVars.addRegionValue(3, 0, 2, 2.9);
    regVars.addRegionValue(3, 0, 3, 3.0);
    regVars.addRegionValue(3, 0, 3, 0.14);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1),  6.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2),  0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 20.29, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1),  43.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2),  86.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 129.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 15.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 26.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 35.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 126.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 227.9, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 328.14, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Cumulative

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Mixed_Var_Type)

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = basic_fipnum();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, true, true, false, });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1, 1.0);
    regVars.addRegionValue(0, 0, 2, 2.0);
    regVars.addRegionValue(0, 0, 3, 3.0);

    regVars.addRegionValue(1, 0, 1, 10.0);
    regVars.addRegionValue(1, 0, 2, 20.0);
    regVars.addRegionValue(1, 0, 3, 30.0);

    regVars.addRegionValue(2, 0, 1, 15.0);
    regVars.addRegionValue(2, 0, 2, 25.0);
    regVars.addRegionValue(2, 0, 3, 35.0);

    regVars.addRegionValue(3, 0, 1, 125.0);
    regVars.addRegionValue(3, 0, 2, 225.0);
    regVars.addRegionValue(3, 0, 3, 325.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 1,   1.0);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 3,  17.29);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 2, - 0.5);
    regVars.addRegionValue(0, 0, 2, - 0.5);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(1, 0, 1, 11.0);
    regVars.addRegionValue(1, 0, 2, 22.0);
    regVars.addRegionValue(1, 0, 3, 33.0);

    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);
    regVars.addRegionValue(2, 0, 2, 0.25);

    regVars.addRegionValue(3, 0, 1, 1.7);
    regVars.addRegionValue(3, 0, 2, 2.9);
    regVars.addRegionValue(3, 0, 3, 3.0);
    regVars.addRegionValue(3, 0, 3, 0.14);

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 1),   5.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), - 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3),  17.29, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 1),  43.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2),  86.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 129.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 1), 15.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 26.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 35.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 1), 1.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 2), 2.9, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(0, 3), 3.14, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Mixed_Var_Type

BOOST_AUTO_TEST_SUITE_END()     // Multi_Variable

BOOST_AUTO_TEST_SUITE_END()     // Single_RegSet

// ########################################################################

BOOST_AUTO_TEST_SUITE(Multi_RegSet)

namespace {
    Opm::data::RegionsetVariableDescriptor regionSets()
    {
        auto descr = Opm::data::RegionsetVariableDescriptor{};

        descr.prepareDescriptorSet();

        descr.addRegionSet(0);  // FIELD

        descr.addRegionSet(3);  // Supports regions 1, 2, 3.  Strictly
                                // speaking also region 0.

        descr.addRegionSet(2);  // FIPABC

        descr.finaliseDescriptorSet();

        return descr;
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Single_Variable)

BOOST_AUTO_TEST_SUITE(Non_Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 42.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1), 1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), 3.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), 17.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2), 29.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 6.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1),  4.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2),  8.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), 12.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), 42.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2), 28.5, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 1.618);

    regVars.addRegionValue(0, 1, 1,  0.75);
    regVars.addRegionValue(0, 1, 3, -0.5);

    regVars.addRegionValue(0, 2, 1, 2.71828);
    regVars.addRegionValue(0, 2, 2, -3.1415);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 1.618, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1),  0.75, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2),  0.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), -0.5, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), 2.71828, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2), -3.1415, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0,  1.0);
    regVars.addRegionValue(0, 0, 0, -2.0);
    regVars.addRegionValue(0, 0, 0,  3.0);

    regVars.addRegionValue(0, 1, 1, 0.25);
    regVars.addRegionValue(0, 1, 1, 0.50);
    regVars.addRegionValue(0, 1, 1, 0.75);
    regVars.addRegionValue(0, 1, 1, 1.00);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 1.5);
    regVars.addRegionValue(0, 1, 2, 1.0);
    regVars.addRegionValue(0, 1, 2, 0.5);

    regVars.addRegionValue(0, 1, 3,  3.0);
    regVars.addRegionValue(0, 1, 3,  6.0);
    regVars.addRegionValue(0, 1, 3, -9.0);
    regVars.addRegionValue(0, 1, 3, 12.5);

    regVars.addRegionValue(0, 2, 1,  8.0);
    regVars.addRegionValue(0, 2, 2,  1.23);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, -9.1011);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 2.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1),  2.5, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2),  5.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), 12.5, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), -1.1011, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2),  0.73, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Non_Cumulative

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 42.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1), 1.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), 3.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), 17.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2), 29.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 6.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1),  4.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2),  8.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), 12.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), 42.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2), 28.5, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 1.618);

    regVars.addRegionValue(0, 1, 1,  0.75);
    regVars.addRegionValue(0, 1, 3, -0.5);

    regVars.addRegionValue(0, 2, 1, 2.71828);
    regVars.addRegionValue(0, 2, 2, -3.1415);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 43.618, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1), 1.75, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2), 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), 2.5, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), 19.71828, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2), 25.8585, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true });

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    regVars.addRegionValue(0, 0, 0,  1.0);
    regVars.addRegionValue(0, 0, 0, -2.0);
    regVars.addRegionValue(0, 0, 0,  3.0);

    regVars.addRegionValue(0, 1, 1, 0.25);
    regVars.addRegionValue(0, 1, 1, 0.50);
    regVars.addRegionValue(0, 1, 1, 0.75);
    regVars.addRegionValue(0, 1, 1, 1.00);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 1.5);
    regVars.addRegionValue(0, 1, 2, 1.0);
    regVars.addRegionValue(0, 1, 2, 0.5);

    regVars.addRegionValue(0, 1, 3,  3.0);
    regVars.addRegionValue(0, 1, 3,  6.0);
    regVars.addRegionValue(0, 1, 3, -9.0);
    regVars.addRegionValue(0, 1, 3, 12.5);

    regVars.addRegionValue(0, 2, 1,  8.0);
    regVars.addRegionValue(0, 2, 2,  1.23);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, -9.1011);

    regVars.commitValues();

    const auto v = regVars.values(0);

    BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

    BOOST_CHECK_CLOSE(v->element(0, 0), 8.0, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(1, 1),  6.5, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 2), 13.0, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(1, 3), 24.5, 1.0e-8);

    BOOST_CHECK_CLOSE(v->element(2, 1), 40.8989, 1.0e-8);
    BOOST_CHECK_CLOSE(v->element(2, 2), 29.23, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Cumulative

BOOST_AUTO_TEST_SUITE_END()     // Single_Variable

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Multi_Variable)

BOOST_AUTO_TEST_SUITE(Non_Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 5.0);
    regVars.addRegionValue(1, 1, 2, 6.0);
    regVars.addRegionValue(1, 1, 3, 7.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 11.22);

    regVars.addRegionValue(2, 1, 1, 2.34);
    regVars.addRegionValue(2, 1, 2, 3.45);
    regVars.addRegionValue(2, 1, 3, 4.56);

    regVars.addRegionValue(2, 2, 1, 7.89);
    regVars.addRegionValue(2, 2, 2, 8.910);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.1);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 3, 0.4);

    regVars.addRegionValue(3, 2, 1, 0.5);
    regVars.addRegionValue(3, 2, 2, 0.6);

    // --------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 42.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 3.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 17.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 29.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 12.34, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 5.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 6.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 8.9, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 10.11, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 11.22, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 2.34, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 3.45, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 4.56, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 7.89, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 8.910, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.1, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 0.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 0.3, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 0.4, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 0.6, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.1);
    regVars.addRegionValue(1, 0, 0, 2.2);
    regVars.addRegionValue(1, 0, 0, 3.3);

    regVars.addRegionValue(1, 1, 1, 0.25);
    regVars.addRegionValue(1, 1, 1, 1.0);
    regVars.addRegionValue(1, 1, 1, 1.75);
    regVars.addRegionValue(1, 1, 1, 2.5);

    regVars.addRegionValue(1, 1, 2, 2.1);
    regVars.addRegionValue(1, 1, 2, 2.2);
    regVars.addRegionValue(1, 1, 2, 2.3);
    regVars.addRegionValue(1, 1, 2, 2.4);

    regVars.addRegionValue(1, 1, 3, 3.5);
    regVars.addRegionValue(1, 1, 3, 3.4);
    regVars.addRegionValue(1, 1, 3, 3.3);
    regVars.addRegionValue(1, 1, 3, 3.2);

    regVars.addRegionValue(1, 2, 1, 4.01);
    regVars.addRegionValue(1, 2, 2, 3.02);
    regVars.addRegionValue(1, 2, 2, 2.03);
    regVars.addRegionValue(1, 2, 1, 1.04);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 10.0);
    regVars.addRegionValue(2, 0, 0, 20.0);
    regVars.addRegionValue(2, 0, 0, 30.0);

    regVars.addRegionValue(2, 1, 1, 30.25);
    regVars.addRegionValue(2, 1, 1, 31.0);
    regVars.addRegionValue(2, 1, 1, 31.75);
    regVars.addRegionValue(2, 1, 1, 32.5);

    regVars.addRegionValue(2, 1, 2, 52.1);
    regVars.addRegionValue(2, 1, 2, 52.2);
    regVars.addRegionValue(2, 1, 2, 52.3);
    regVars.addRegionValue(2, 1, 2, 52.4);

    regVars.addRegionValue(2, 1, 3, 93.5);
    regVars.addRegionValue(2, 1, 3, 93.4);
    regVars.addRegionValue(2, 1, 3, 93.3);
    regVars.addRegionValue(2, 1, 3, 93.2);

    regVars.addRegionValue(2, 2, 1, 1004.01);
    regVars.addRegionValue(2, 2, 2, 1003.02);
    regVars.addRegionValue(2, 2, 2, 1002.03);
    regVars.addRegionValue(2, 2, 1, 1001.04);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 10.0);
    regVars.addRegionValue(3, 0, 0, -20.0);
    regVars.addRegionValue(3, 0, 0, 30.0);

    regVars.addRegionValue(3, 1, 1, 30.25);
    regVars.addRegionValue(3, 1, 1, -31.0);
    regVars.addRegionValue(3, 1, 1, -31.75);
    regVars.addRegionValue(3, 1, 1, 32.5);

    regVars.addRegionValue(3, 1, 2, -52.1);
    regVars.addRegionValue(3, 1, 2, -52.2);
    regVars.addRegionValue(3, 1, 2, -52.3);
    regVars.addRegionValue(3, 1, 2, 52.4);

    regVars.addRegionValue(3, 1, 3, 93.5);
    regVars.addRegionValue(3, 1, 3, 93.4);
    regVars.addRegionValue(3, 1, 3, 93.3);
    regVars.addRegionValue(3, 1, 3, -93.2);

    regVars.addRegionValue(3, 2, 1, 1004.01);
    regVars.addRegionValue(3, 2, 2, 1003.02);
    regVars.addRegionValue(3, 2, 2, -1002.03);
    regVars.addRegionValue(3, 2, 1, -1001.04);

    // --------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 6, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  4.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2),  8.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 12.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 42.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 28.5, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 6.6, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  5.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2),  9.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 13.4, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 5.05, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 5.05, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 60.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 125.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 209.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 373.4, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 2005.05, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 2005.05, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 20.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),    0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), -104.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3),  187.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 2.97, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 0.99, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 5.0);
    regVars.addRegionValue(1, 1, 2, 6.0);
    regVars.addRegionValue(1, 1, 3, 7.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 11.22);

    regVars.addRegionValue(2, 1, 1, 2.34);
    regVars.addRegionValue(2, 1, 2, 3.45);
    regVars.addRegionValue(2, 1, 3, 4.56);

    regVars.addRegionValue(2, 2, 1, 7.89);
    regVars.addRegionValue(2, 2, 2, 8.910);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.1);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 3, 0.4);

    regVars.addRegionValue(3, 2, 1, 0.5);
    regVars.addRegionValue(3, 2, 2, 0.6);

    // --------------------------------------------------------------------

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 3.1415926);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, -2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 1.7);
    regVars.addRegionValue(0, 2, 2, 2.9);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.234);

    regVars.addRegionValue(1, 1, 1, 5.678);
    regVars.addRegionValue(1, 1, 2, 6.789);
    regVars.addRegionValue(1, 1, 3, 7.8910);

    regVars.addRegionValue(1, 2, 1, 11.12);
    regVars.addRegionValue(1, 2, 2, 12.1314);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 10.987);

    regVars.addRegionValue(2, 1, 1, 9.876);
    regVars.addRegionValue(2, 1, 2, 8.765);
    regVars.addRegionValue(2, 1, 3, 7.654);

    regVars.addRegionValue(2, 2, 1, 6.543);
    regVars.addRegionValue(2, 2, 2, 5.432);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.125);

    regVars.addRegionValue(3, 1, 1, 0.25);
    regVars.addRegionValue(3, 1, 2, 0.375);
    regVars.addRegionValue(3, 1, 3, 0.5);

    regVars.addRegionValue(3, 2, 1, 0.625);
    regVars.addRegionValue(3, 2, 2, 0.75);

    // --------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 3.1415926, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), -2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 3.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 1.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 2.9, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 1.234, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 5.678, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 6.789, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.8910, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 11.12, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 12.1314, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 10.987, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 9.876, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 8.765, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.654, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 6.543, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 5.432, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.125, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 0.25, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 0.375, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 0.5, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 0.625, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 0.75, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, false, false, });

    regVars.prepareValueAccumulation();

    // ------------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 2.0);
    regVars.addRegionValue(1, 1, 1, 2.0);
    regVars.addRegionValue(1, 1, 1, 1.0);

    regVars.addRegionValue(1, 1, 2, 1.0);
    regVars.addRegionValue(1, 1, 2, 2.0);
    regVars.addRegionValue(1, 1, 2, 3.0);
    regVars.addRegionValue(1, 1, 2, 4.0);

    regVars.addRegionValue(1, 1, 3,  2.0);
    regVars.addRegionValue(1, 1, 3,  1.0);
    regVars.addRegionValue(1, 1, 3, -1.0);
    regVars.addRegionValue(1, 1, 3,  3.0);
    regVars.addRegionValue(1, 1, 3, -2.0);
    regVars.addRegionValue(1, 1, 3,  4.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 1.234);

    regVars.addRegionValue(2, 1, 1, 20.0);
    regVars.addRegionValue(2, 1, 1, 20.0);
    regVars.addRegionValue(2, 1, 1, 10.0);

    regVars.addRegionValue(2, 1, 2, 10.0);
    regVars.addRegionValue(2, 1, 2, 20.0);
    regVars.addRegionValue(2, 1, 2, 30.0);
    regVars.addRegionValue(2, 1, 2, 40.0);

    regVars.addRegionValue(2, 1, 3,  20.0);
    regVars.addRegionValue(2, 1, 3,  10.0);
    regVars.addRegionValue(2, 1, 3, -10.0);
    regVars.addRegionValue(2, 1, 3,  30.0);
    regVars.addRegionValue(2, 1, 3, -20.0);
    regVars.addRegionValue(2, 1, 3,  40.0);

    regVars.addRegionValue(2, 2, 1, 89.0);
    regVars.addRegionValue(2, 2, 2, 101.1);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 123.4);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 1, 0.1);

    regVars.addRegionValue(3, 1, 2, 0.1);
    regVars.addRegionValue(3, 1, 2, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 2, 0.4);

    regVars.addRegionValue(3, 1, 3,  0.2);
    regVars.addRegionValue(3, 1, 3,  0.1);
    regVars.addRegionValue(3, 1, 3, -0.1);
    regVars.addRegionValue(3, 1, 3,  0.3);
    regVars.addRegionValue(3, 1, 3, -0.2);
    regVars.addRegionValue(3, 1, 3,  0.4);

    regVars.addRegionValue(3, 2, 1, 0.89);
    regVars.addRegionValue(3, 2, 2, 0.1011);

    // ------------------------------------------------------------------------

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    // ------------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.1);
    regVars.addRegionValue(0, 1, 3, 3.2);
    regVars.addRegionValue(0, 1, 3, 3.3);
    regVars.addRegionValue(0, 1, 3, 3.4);

    regVars.addRegionValue(0, 2, 1,  1.72);
    regVars.addRegionValue(0, 2, 2,  9.0);
    regVars.addRegionValue(0, 2, 2, -1.5);
    regVars.addRegionValue(0, 2, 1,  2.48);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.234);
    regVars.addRegionValue(1, 0, 0, 12.34);
    regVars.addRegionValue(1, 0, 0, 123.4);
    regVars.addRegionValue(1, 0, 0, 1234);

    regVars.addRegionValue(1, 1, 1, 2.03);
    regVars.addRegionValue(1, 1, 1, 2.23);
    regVars.addRegionValue(1, 1, 1, 1.13);

    regVars.addRegionValue(1, 1, 2, 1.005);
    regVars.addRegionValue(1, 1, 2, 2.005);
    regVars.addRegionValue(1, 1, 2, 3.005);
    regVars.addRegionValue(1, 1, 2, 4.005);

    regVars.addRegionValue(1, 1, 3,  2.1729);
    regVars.addRegionValue(1, 1, 3,  1.1729);
    regVars.addRegionValue(1, 1, 3, -1.1729);
    regVars.addRegionValue(1, 1, 3,  3.1729);
    regVars.addRegionValue(1, 1, 3, -2.1729);
    regVars.addRegionValue(1, 1, 3,  4.1729);

    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);

    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0,  1.234);
    regVars.addRegionValue(2, 0, 0, -1.234 / 2);

    regVars.addRegionValue(2, 1, 1, 25.2);
    regVars.addRegionValue(2, 1, 1, 25.2);
    regVars.addRegionValue(2, 1, 1, 15.2);

    regVars.addRegionValue(2, 1, 2, 14.3);
    regVars.addRegionValue(2, 1, 2, 24.3);
    regVars.addRegionValue(2, 1, 2, 34.3);
    regVars.addRegionValue(2, 1, 2, 44.3);

    regVars.addRegionValue(2, 1, 3,  23.4);
    regVars.addRegionValue(2, 1, 3,  13.4);
    regVars.addRegionValue(2, 1, 3, -13.4);
    regVars.addRegionValue(2, 1, 3,  33.4);
    regVars.addRegionValue(2, 1, 3, -23.4);
    regVars.addRegionValue(2, 1, 3,  43.4);

    regVars.addRegionValue(2, 2, 1, 89.5);
    regVars.addRegionValue(2, 2, 1, 89.5 / 2);
    regVars.addRegionValue(2, 2, 1, 89.5 / 3);
    regVars.addRegionValue(2, 2, 1, 89.5 / 4);

    regVars.addRegionValue(2, 2, 2, 101.15);
    regVars.addRegionValue(2, 2, 2, 101.15 * 2);
    regVars.addRegionValue(2, 2, 2, 101.15 * 3);
    regVars.addRegionValue(2, 2, 2, 101.15 * 4);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 100.0);
    regVars.addRegionValue(3, 0, 0,  20.0);
    regVars.addRegionValue(3, 0, 0,   3.0);
    regVars.addRegionValue(3, 0, 0,   0.4);
    regVars.addRegionValue(3, 0, 0,   0.07);

    regVars.addRegionValue(3, 1, 1, 0.27);
    regVars.addRegionValue(3, 1, 1, 0.27);
    regVars.addRegionValue(3, 1, 1, 0.17);

    regVars.addRegionValue(3, 1, 2, 0.17);
    regVars.addRegionValue(3, 1, 2, 0.27);
    regVars.addRegionValue(3, 1, 2, 0.37);
    regVars.addRegionValue(3, 1, 2, 0.47);

    regVars.addRegionValue(3, 1, 3,  0.27);
    regVars.addRegionValue(3, 1, 3,  0.17);
    regVars.addRegionValue(3, 1, 3, -0.17);
    regVars.addRegionValue(3, 1, 3,  0.37);
    regVars.addRegionValue(3, 1, 3, -0.27);
    regVars.addRegionValue(3, 1, 3,  0.47);

    regVars.addRegionValue(3, 2, 1, 0.897);
    regVars.addRegionValue(3, 2, 1, 1.0);

    regVars.addRegionValue(3, 2, 2,   0.10117);
    regVars.addRegionValue(3, 2, 2,  13.0);
    regVars.addRegionValue(3, 2, 2, - 0.10117);

    // ------------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.9468, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2),  6.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 13.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 4.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 7.5, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 1370.974, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 5.39, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 10.02, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.3458, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 53.46066, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 97.0512, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.617, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  65.6, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 117.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3),  76.8, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 186.45833333333334, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 1011.5, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 123.47, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 0.71, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 1.28, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 0.84, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1),  1.897, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 13.0, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Non_Cumulative

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Cumulative)

BOOST_AUTO_TEST_CASE(Single_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 5.0);
    regVars.addRegionValue(1, 1, 2, 6.0);
    regVars.addRegionValue(1, 1, 3, 7.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 11.22);

    regVars.addRegionValue(2, 1, 1, 2.34);
    regVars.addRegionValue(2, 1, 2, 3.45);
    regVars.addRegionValue(2, 1, 3, 4.56);

    regVars.addRegionValue(2, 2, 1, 7.89);
    regVars.addRegionValue(2, 2, 2, 8.910);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.1);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 3, 0.4);

    regVars.addRegionValue(3, 2, 1, 0.5);
    regVars.addRegionValue(3, 2, 2, 0.6);

    // --------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 42.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 1.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 3.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 17.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 29.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 12.34, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 5.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 6.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 8.9, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 10.11, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 11.22, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 2.34, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 3.45, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 4.56, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 7.89, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 8.910, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.1, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 0.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 0.3, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 0.4, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 0.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 0.6, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Single_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.1);
    regVars.addRegionValue(1, 0, 0, 2.2);
    regVars.addRegionValue(1, 0, 0, 3.3);

    regVars.addRegionValue(1, 1, 1, 0.25);
    regVars.addRegionValue(1, 1, 1, 1.0);
    regVars.addRegionValue(1, 1, 1, 1.75);
    regVars.addRegionValue(1, 1, 1, 2.5);

    regVars.addRegionValue(1, 1, 2, 2.1);
    regVars.addRegionValue(1, 1, 2, 2.2);
    regVars.addRegionValue(1, 1, 2, 2.3);
    regVars.addRegionValue(1, 1, 2, 2.4);

    regVars.addRegionValue(1, 1, 3, 3.5);
    regVars.addRegionValue(1, 1, 3, 3.4);
    regVars.addRegionValue(1, 1, 3, 3.3);
    regVars.addRegionValue(1, 1, 3, 3.2);

    regVars.addRegionValue(1, 2, 1, 4.01);
    regVars.addRegionValue(1, 2, 2, 3.02);
    regVars.addRegionValue(1, 2, 2, 2.03);
    regVars.addRegionValue(1, 2, 1, 1.04);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 10.0);
    regVars.addRegionValue(2, 0, 0, 20.0);
    regVars.addRegionValue(2, 0, 0, 30.0);

    regVars.addRegionValue(2, 1, 1, 30.25);
    regVars.addRegionValue(2, 1, 1, 31.0);
    regVars.addRegionValue(2, 1, 1, 31.75);
    regVars.addRegionValue(2, 1, 1, 32.5);

    regVars.addRegionValue(2, 1, 2, 52.1);
    regVars.addRegionValue(2, 1, 2, 52.2);
    regVars.addRegionValue(2, 1, 2, 52.3);
    regVars.addRegionValue(2, 1, 2, 52.4);

    regVars.addRegionValue(2, 1, 3, 93.5);
    regVars.addRegionValue(2, 1, 3, 93.4);
    regVars.addRegionValue(2, 1, 3, 93.3);
    regVars.addRegionValue(2, 1, 3, 93.2);

    regVars.addRegionValue(2, 2, 1, 1004.01);
    regVars.addRegionValue(2, 2, 2, 1003.02);
    regVars.addRegionValue(2, 2, 2, 1002.03);
    regVars.addRegionValue(2, 2, 1, 1001.04);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 10.0);
    regVars.addRegionValue(3, 0, 0, -20.0);
    regVars.addRegionValue(3, 0, 0, 30.0);

    regVars.addRegionValue(3, 1, 1, 30.25);
    regVars.addRegionValue(3, 1, 1, -31.0);
    regVars.addRegionValue(3, 1, 1, -31.75);
    regVars.addRegionValue(3, 1, 1, 32.5);

    regVars.addRegionValue(3, 1, 2, -52.1);
    regVars.addRegionValue(3, 1, 2, -52.2);
    regVars.addRegionValue(3, 1, 2, -52.3);
    regVars.addRegionValue(3, 1, 2, 52.4);

    regVars.addRegionValue(3, 1, 3, 93.5);
    regVars.addRegionValue(3, 1, 3, 93.4);
    regVars.addRegionValue(3, 1, 3, 93.3);
    regVars.addRegionValue(3, 1, 3, -93.2);

    regVars.addRegionValue(3, 2, 1, 1004.01);
    regVars.addRegionValue(3, 2, 2, 1003.02);
    regVars.addRegionValue(3, 2, 2, -1002.03);
    regVars.addRegionValue(3, 2, 1, -1001.04);

    // --------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 6, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  4.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2),  8.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 12.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 42.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 28.5, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 6.6, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  5.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2),  9.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 13.4, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 5.05, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 5.05, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 60.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 125.5, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 209.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 373.4, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 2005.05, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 2005.05, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 20.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),    0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), -104.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3),  187.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 2.97, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 0.99, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 5.0);
    regVars.addRegionValue(1, 1, 2, 6.0);
    regVars.addRegionValue(1, 1, 3, 7.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 11.22);

    regVars.addRegionValue(2, 1, 1, 2.34);
    regVars.addRegionValue(2, 1, 2, 3.45);
    regVars.addRegionValue(2, 1, 3, 4.56);

    regVars.addRegionValue(2, 2, 1, 7.89);
    regVars.addRegionValue(2, 2, 2, 8.910);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.1);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 3, 0.4);

    regVars.addRegionValue(3, 2, 1, 0.5);
    regVars.addRegionValue(3, 2, 2, 0.6);

    // --------------------------------------------------------------------

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 3.1415926);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, -2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 1.7);
    regVars.addRegionValue(0, 2, 2, 2.9);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.234);

    regVars.addRegionValue(1, 1, 1, 5.678);
    regVars.addRegionValue(1, 1, 2, 6.789);
    regVars.addRegionValue(1, 1, 3, 7.8910);

    regVars.addRegionValue(1, 2, 1, 11.12);
    regVars.addRegionValue(1, 2, 2, 12.1314);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 10.987);

    regVars.addRegionValue(2, 1, 1, 9.876);
    regVars.addRegionValue(2, 1, 2, 8.765);
    regVars.addRegionValue(2, 1, 3, 7.654);

    regVars.addRegionValue(2, 2, 1, 6.543);
    regVars.addRegionValue(2, 2, 2, 5.432);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.125);

    regVars.addRegionValue(3, 1, 1, 0.25);
    regVars.addRegionValue(3, 1, 2, 0.375);
    regVars.addRegionValue(3, 1, 3, 0.5);

    regVars.addRegionValue(3, 2, 1, 0.625);
    regVars.addRegionValue(3, 2, 2, 0.75);

    // --------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 45.1415926, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 6.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 18.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 31.9, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 13.574, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 10.678, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 12.789, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 14.8910, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 20.02, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 22.2414, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 22.207, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 12.216, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 12.215, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 12.214, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 14.433, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 14.342, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.225, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 0.45, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 0.675, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 0.9, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 1.125, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 1.35, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, true, true, true, });

    regVars.prepareValueAccumulation();

    // ------------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 2.0);
    regVars.addRegionValue(1, 1, 1, 2.0);
    regVars.addRegionValue(1, 1, 1, 1.0);

    regVars.addRegionValue(1, 1, 2, 1.0);
    regVars.addRegionValue(1, 1, 2, 2.0);
    regVars.addRegionValue(1, 1, 2, 3.0);
    regVars.addRegionValue(1, 1, 2, 4.0);

    regVars.addRegionValue(1, 1, 3,  2.0);
    regVars.addRegionValue(1, 1, 3,  1.0);
    regVars.addRegionValue(1, 1, 3, -1.0);
    regVars.addRegionValue(1, 1, 3,  3.0);
    regVars.addRegionValue(1, 1, 3, -2.0);
    regVars.addRegionValue(1, 1, 3,  4.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 1.234);

    regVars.addRegionValue(2, 1, 1, 20.0);
    regVars.addRegionValue(2, 1, 1, 20.0);
    regVars.addRegionValue(2, 1, 1, 10.0);

    regVars.addRegionValue(2, 1, 2, 10.0);
    regVars.addRegionValue(2, 1, 2, 20.0);
    regVars.addRegionValue(2, 1, 2, 30.0);
    regVars.addRegionValue(2, 1, 2, 40.0);

    regVars.addRegionValue(2, 1, 3,  20.0);
    regVars.addRegionValue(2, 1, 3,  10.0);
    regVars.addRegionValue(2, 1, 3, -10.0);
    regVars.addRegionValue(2, 1, 3,  30.0);
    regVars.addRegionValue(2, 1, 3, -20.0);
    regVars.addRegionValue(2, 1, 3,  40.0);

    regVars.addRegionValue(2, 2, 1, 89.0);
    regVars.addRegionValue(2, 2, 2, 101.1);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 123.4);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 1, 0.1);

    regVars.addRegionValue(3, 1, 2, 0.1);
    regVars.addRegionValue(3, 1, 2, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 2, 0.4);

    regVars.addRegionValue(3, 1, 3,  0.2);
    regVars.addRegionValue(3, 1, 3,  0.1);
    regVars.addRegionValue(3, 1, 3, -0.1);
    regVars.addRegionValue(3, 1, 3,  0.3);
    regVars.addRegionValue(3, 1, 3, -0.2);
    regVars.addRegionValue(3, 1, 3,  0.4);

    regVars.addRegionValue(3, 2, 1, 0.89);
    regVars.addRegionValue(3, 2, 2, 0.1011);

    // ------------------------------------------------------------------------

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    // ------------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.1);
    regVars.addRegionValue(0, 1, 3, 3.2);
    regVars.addRegionValue(0, 1, 3, 3.3);
    regVars.addRegionValue(0, 1, 3, 3.4);

    regVars.addRegionValue(0, 2, 1,  1.72);
    regVars.addRegionValue(0, 2, 2,  9.0);
    regVars.addRegionValue(0, 2, 2, -1.5);
    regVars.addRegionValue(0, 2, 1,  2.48);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.234);
    regVars.addRegionValue(1, 0, 0, 12.34);
    regVars.addRegionValue(1, 0, 0, 123.4);
    regVars.addRegionValue(1, 0, 0, 1234);

    regVars.addRegionValue(1, 1, 1, 2.03);
    regVars.addRegionValue(1, 1, 1, 2.23);
    regVars.addRegionValue(1, 1, 1, 1.13);

    regVars.addRegionValue(1, 1, 2, 1.005);
    regVars.addRegionValue(1, 1, 2, 2.005);
    regVars.addRegionValue(1, 1, 2, 3.005);
    regVars.addRegionValue(1, 1, 2, 4.005);

    regVars.addRegionValue(1, 1, 3,  2.1729);
    regVars.addRegionValue(1, 1, 3,  1.1729);
    regVars.addRegionValue(1, 1, 3, -1.1729);
    regVars.addRegionValue(1, 1, 3,  3.1729);
    regVars.addRegionValue(1, 1, 3, -2.1729);
    regVars.addRegionValue(1, 1, 3,  4.1729);

    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);

    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0,  1.234);
    regVars.addRegionValue(2, 0, 0, -1.234 / 2);

    regVars.addRegionValue(2, 1, 1, 25.2);
    regVars.addRegionValue(2, 1, 1, 25.2);
    regVars.addRegionValue(2, 1, 1, 15.2);

    regVars.addRegionValue(2, 1, 2, 14.3);
    regVars.addRegionValue(2, 1, 2, 24.3);
    regVars.addRegionValue(2, 1, 2, 34.3);
    regVars.addRegionValue(2, 1, 2, 44.3);

    regVars.addRegionValue(2, 1, 3,  23.4);
    regVars.addRegionValue(2, 1, 3,  13.4);
    regVars.addRegionValue(2, 1, 3, -13.4);
    regVars.addRegionValue(2, 1, 3,  33.4);
    regVars.addRegionValue(2, 1, 3, -23.4);
    regVars.addRegionValue(2, 1, 3,  43.4);

    regVars.addRegionValue(2, 2, 1, 89.5);
    regVars.addRegionValue(2, 2, 1, 89.5 / 2);
    regVars.addRegionValue(2, 2, 1, 89.5 / 3);
    regVars.addRegionValue(2, 2, 1, 89.5 / 4);

    regVars.addRegionValue(2, 2, 2, 101.15);
    regVars.addRegionValue(2, 2, 2, 101.15 * 2);
    regVars.addRegionValue(2, 2, 2, 101.15 * 3);
    regVars.addRegionValue(2, 2, 2, 101.15 * 4);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 100.0);
    regVars.addRegionValue(3, 0, 0,  20.0);
    regVars.addRegionValue(3, 0, 0,   3.0);
    regVars.addRegionValue(3, 0, 0,   0.4);
    regVars.addRegionValue(3, 0, 0,   0.07);

    regVars.addRegionValue(3, 1, 1, 0.27);
    regVars.addRegionValue(3, 1, 1, 0.27);
    regVars.addRegionValue(3, 1, 1, 0.17);

    regVars.addRegionValue(3, 1, 2, 0.17);
    regVars.addRegionValue(3, 1, 2, 0.27);
    regVars.addRegionValue(3, 1, 2, 0.37);
    regVars.addRegionValue(3, 1, 2, 0.47);

    regVars.addRegionValue(3, 1, 3,  0.27);
    regVars.addRegionValue(3, 1, 3,  0.17);
    regVars.addRegionValue(3, 1, 3, -0.17);
    regVars.addRegionValue(3, 1, 3,  0.37);
    regVars.addRegionValue(3, 1, 3, -0.27);
    regVars.addRegionValue(3, 1, 3,  0.47);

    regVars.addRegionValue(3, 2, 1, 0.897);
    regVars.addRegionValue(3, 2, 1, 1.0);

    regVars.addRegionValue(3, 2, 2,   0.10117);
    regVars.addRegionValue(3, 2, 2,  13.0);
    regVars.addRegionValue(3, 2, 2, - 0.10117);

    // ------------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 6.9468, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  6.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 14.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 25.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 46.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 36.0, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 1383.314, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 10.39, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 20.02, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 14.3458, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 62.36066, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 107.1612, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 1.851, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 115.6, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 217.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 146.8, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 275.45833333333334, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 1112.6, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 246.87, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 1.21, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 2.28, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 1.54, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1),  2.787, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 13.1011, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Cumulative

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Var_Type)

BOOST_AUTO_TEST_CASE(Multi_Accum_Unique_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { true, false, false, true, });

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 42.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 5.0);
    regVars.addRegionValue(1, 1, 2, 6.0);
    regVars.addRegionValue(1, 1, 3, 7.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 11.22);

    regVars.addRegionValue(2, 1, 1, 2.34);
    regVars.addRegionValue(2, 1, 2, 3.45);
    regVars.addRegionValue(2, 1, 3, 4.56);

    regVars.addRegionValue(2, 2, 1, 7.89);
    regVars.addRegionValue(2, 2, 2, 8.910);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.1);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 3, 0.4);

    regVars.addRegionValue(3, 2, 1, 0.5);
    regVars.addRegionValue(3, 2, 2, 0.6);

    // --------------------------------------------------------------------

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    // --------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 3.1415926);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 2, -2.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 1.7);
    regVars.addRegionValue(0, 2, 2, 2.9);

    // --------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.234);

    regVars.addRegionValue(1, 1, 1, 5.678);
    regVars.addRegionValue(1, 1, 2, 6.789);
    regVars.addRegionValue(1, 1, 3, 7.8910);

    regVars.addRegionValue(1, 2, 1, 11.12);
    regVars.addRegionValue(1, 2, 2, 12.1314);

    // --------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 10.987);

    regVars.addRegionValue(2, 1, 1, 9.876);
    regVars.addRegionValue(2, 1, 2, 8.765);
    regVars.addRegionValue(2, 1, 3, 7.654);

    regVars.addRegionValue(2, 2, 1, 6.543);
    regVars.addRegionValue(2, 2, 2, 5.432);

    // --------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 0.125);

    regVars.addRegionValue(3, 1, 1, 0.25);
    regVars.addRegionValue(3, 1, 2, 0.375);
    regVars.addRegionValue(3, 1, 3, 0.5);

    regVars.addRegionValue(3, 2, 1, 0.625);
    regVars.addRegionValue(3, 2, 2, 0.75);

    // --------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 45.1415926, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 0.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 6.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 18.7, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 31.9, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 1.234, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 5.678, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 6.789, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.8910, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 11.12, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 12.1314, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 10.987, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 9.876, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 8.765, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.654, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 6.543, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 5.432, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.225, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 0.45, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 0.675, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 0.9, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 1.125, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 1.35, 1.0e-8);
    }
}

BOOST_AUTO_TEST_CASE(Multi_Accum_Repeated_RegIx)
{
    // Descriptor must outlive region variables.
    const auto descr = regionSets();

    auto regVars = Opm::data::RegionVariableValues{};

    regVars.defineVariables(descr, std::vector { false, false, true, true, });

    regVars.prepareValueAccumulation();

    // ------------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 1.0);
    regVars.addRegionValue(0, 0, 0, 2.0);
    regVars.addRegionValue(0, 0, 0, 3.0);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);
    regVars.addRegionValue(0, 1, 3, 3.0);

    regVars.addRegionValue(0, 2, 1, 17.0);
    regVars.addRegionValue(0, 2, 2, 29.0);
    regVars.addRegionValue(0, 2, 2, -0.5);
    regVars.addRegionValue(0, 2, 1, 25.0);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 12.34);

    regVars.addRegionValue(1, 1, 1, 2.0);
    regVars.addRegionValue(1, 1, 1, 2.0);
    regVars.addRegionValue(1, 1, 1, 1.0);

    regVars.addRegionValue(1, 1, 2, 1.0);
    regVars.addRegionValue(1, 1, 2, 2.0);
    regVars.addRegionValue(1, 1, 2, 3.0);
    regVars.addRegionValue(1, 1, 2, 4.0);

    regVars.addRegionValue(1, 1, 3,  2.0);
    regVars.addRegionValue(1, 1, 3,  1.0);
    regVars.addRegionValue(1, 1, 3, -1.0);
    regVars.addRegionValue(1, 1, 3,  3.0);
    regVars.addRegionValue(1, 1, 3, -2.0);
    regVars.addRegionValue(1, 1, 3,  4.0);

    regVars.addRegionValue(1, 2, 1, 8.9);
    regVars.addRegionValue(1, 2, 2, 10.11);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0, 1.234);

    regVars.addRegionValue(2, 1, 1, 20.0);
    regVars.addRegionValue(2, 1, 1, 20.0);
    regVars.addRegionValue(2, 1, 1, 10.0);

    regVars.addRegionValue(2, 1, 2, 10.0);
    regVars.addRegionValue(2, 1, 2, 20.0);
    regVars.addRegionValue(2, 1, 2, 30.0);
    regVars.addRegionValue(2, 1, 2, 40.0);

    regVars.addRegionValue(2, 1, 3,  20.0);
    regVars.addRegionValue(2, 1, 3,  10.0);
    regVars.addRegionValue(2, 1, 3, -10.0);
    regVars.addRegionValue(2, 1, 3,  30.0);
    regVars.addRegionValue(2, 1, 3, -20.0);
    regVars.addRegionValue(2, 1, 3,  40.0);

    regVars.addRegionValue(2, 2, 1, 89.0);
    regVars.addRegionValue(2, 2, 2, 101.1);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 123.4);

    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 1, 0.2);
    regVars.addRegionValue(3, 1, 1, 0.1);

    regVars.addRegionValue(3, 1, 2, 0.1);
    regVars.addRegionValue(3, 1, 2, 0.2);
    regVars.addRegionValue(3, 1, 2, 0.3);
    regVars.addRegionValue(3, 1, 2, 0.4);

    regVars.addRegionValue(3, 1, 3,  0.2);
    regVars.addRegionValue(3, 1, 3,  0.1);
    regVars.addRegionValue(3, 1, 3, -0.1);
    regVars.addRegionValue(3, 1, 3,  0.3);
    regVars.addRegionValue(3, 1, 3, -0.2);
    regVars.addRegionValue(3, 1, 3,  0.4);

    regVars.addRegionValue(3, 2, 1, 0.89);
    regVars.addRegionValue(3, 2, 2, 0.1011);

    // ------------------------------------------------------------------------

    regVars.commitValues();

    regVars.prepareValueAccumulation();

    // ------------------------------------------------------------------------

    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);
    regVars.addRegionValue(0, 0, 0, 0.2367);

    regVars.addRegionValue(0, 1, 1, 1.0);
    regVars.addRegionValue(0, 1, 1, 1.0);

    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);
    regVars.addRegionValue(0, 1, 2, 2.0);

    regVars.addRegionValue(0, 1, 3, 3.1);
    regVars.addRegionValue(0, 1, 3, 3.2);
    regVars.addRegionValue(0, 1, 3, 3.3);
    regVars.addRegionValue(0, 1, 3, 3.4);

    regVars.addRegionValue(0, 2, 1,  1.72);
    regVars.addRegionValue(0, 2, 2,  9.0);
    regVars.addRegionValue(0, 2, 2, -1.5);
    regVars.addRegionValue(0, 2, 1,  2.48);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(1, 0, 0, 1.234);
    regVars.addRegionValue(1, 0, 0, 12.34);
    regVars.addRegionValue(1, 0, 0, 123.4);
    regVars.addRegionValue(1, 0, 0, 1234);

    regVars.addRegionValue(1, 1, 1, 2.03);
    regVars.addRegionValue(1, 1, 1, 2.23);
    regVars.addRegionValue(1, 1, 1, 1.13);

    regVars.addRegionValue(1, 1, 2, 1.005);
    regVars.addRegionValue(1, 1, 2, 2.005);
    regVars.addRegionValue(1, 1, 2, 3.005);
    regVars.addRegionValue(1, 1, 2, 4.005);

    regVars.addRegionValue(1, 1, 3,  2.1729);
    regVars.addRegionValue(1, 1, 3,  1.1729);
    regVars.addRegionValue(1, 1, 3, -1.1729);
    regVars.addRegionValue(1, 1, 3,  3.1729);
    regVars.addRegionValue(1, 1, 3, -2.1729);
    regVars.addRegionValue(1, 1, 3,  4.1729);

    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);
    regVars.addRegionValue(1, 2, 1,  8.91011);

    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);
    regVars.addRegionValue(1, 2, 2, 12.1314);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(2, 0, 0,  1.234);
    regVars.addRegionValue(2, 0, 0, -1.234 / 2);

    regVars.addRegionValue(2, 1, 1, 25.2);
    regVars.addRegionValue(2, 1, 1, 25.2);
    regVars.addRegionValue(2, 1, 1, 15.2);

    regVars.addRegionValue(2, 1, 2, 14.3);
    regVars.addRegionValue(2, 1, 2, 24.3);
    regVars.addRegionValue(2, 1, 2, 34.3);
    regVars.addRegionValue(2, 1, 2, 44.3);

    regVars.addRegionValue(2, 1, 3,  23.4);
    regVars.addRegionValue(2, 1, 3,  13.4);
    regVars.addRegionValue(2, 1, 3, -13.4);
    regVars.addRegionValue(2, 1, 3,  33.4);
    regVars.addRegionValue(2, 1, 3, -23.4);
    regVars.addRegionValue(2, 1, 3,  43.4);

    regVars.addRegionValue(2, 2, 1, 89.5);
    regVars.addRegionValue(2, 2, 1, 89.5 / 2);
    regVars.addRegionValue(2, 2, 1, 89.5 / 3);
    regVars.addRegionValue(2, 2, 1, 89.5 / 4);

    regVars.addRegionValue(2, 2, 2, 101.15);
    regVars.addRegionValue(2, 2, 2, 101.15 * 2);
    regVars.addRegionValue(2, 2, 2, 101.15 * 3);
    regVars.addRegionValue(2, 2, 2, 101.15 * 4);

    // ------------------------------------------------------------------------

    regVars.addRegionValue(3, 0, 0, 100.0);
    regVars.addRegionValue(3, 0, 0,  20.0);
    regVars.addRegionValue(3, 0, 0,   3.0);
    regVars.addRegionValue(3, 0, 0,   0.4);
    regVars.addRegionValue(3, 0, 0,   0.07);

    regVars.addRegionValue(3, 1, 1, 0.27);
    regVars.addRegionValue(3, 1, 1, 0.27);
    regVars.addRegionValue(3, 1, 1, 0.17);

    regVars.addRegionValue(3, 1, 2, 0.17);
    regVars.addRegionValue(3, 1, 2, 0.27);
    regVars.addRegionValue(3, 1, 2, 0.37);
    regVars.addRegionValue(3, 1, 2, 0.47);

    regVars.addRegionValue(3, 1, 3,  0.27);
    regVars.addRegionValue(3, 1, 3,  0.17);
    regVars.addRegionValue(3, 1, 3, -0.17);
    regVars.addRegionValue(3, 1, 3,  0.37);
    regVars.addRegionValue(3, 1, 3, -0.27);
    regVars.addRegionValue(3, 1, 3,  0.47);

    regVars.addRegionValue(3, 2, 1, 0.897);
    regVars.addRegionValue(3, 2, 1, 1.0);

    regVars.addRegionValue(3, 2, 2,   0.10117);
    regVars.addRegionValue(3, 2, 2,  13.0);
    regVars.addRegionValue(3, 2, 2, - 0.10117);

    // ------------------------------------------------------------------------

    regVars.commitValues();

    {
        const auto v = regVars.values(0);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable zero");

        BOOST_CHECK_CLOSE(v->element(0, 0), 0.9468, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1),  2.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2),  6.0, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 13.0, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 4.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 7.5, 1.0e-8);
    }

    {
        const auto v = regVars.values(1);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable one");

        BOOST_CHECK_CLOSE(v->element(0, 0), 1370.974, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 5.39, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 10.02, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 7.3458, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 53.46066, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 97.0512, 1.0e-8);
    }

    {
        const auto v = regVars.values(2);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable two");

        BOOST_CHECK_CLOSE(v->element(0, 0), 1.851, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 115.6, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 217.2, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 146.8, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1), 275.45833333333334, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 1112.6, 1.0e-8);
    }

    {
        const auto v = regVars.values(3);

        BOOST_REQUIRE_MESSAGE(v.has_value(), "There must be a view for variable three");

        BOOST_CHECK_CLOSE(v->element(0, 0), 246.87, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(1, 1), 1.21, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 2), 2.28, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(1, 3), 1.54, 1.0e-8);

        BOOST_CHECK_CLOSE(v->element(2, 1),  2.787, 1.0e-8);
        BOOST_CHECK_CLOSE(v->element(2, 2), 13.1011, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Var_Type

BOOST_AUTO_TEST_SUITE_END()     // Multi_Variable

BOOST_AUTO_TEST_SUITE_END()     // Multi_RegSet
