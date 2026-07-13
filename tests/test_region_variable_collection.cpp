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

#define BOOST_TEST_MODULE Region_Variable_Collection

#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/RegionVariableCollection.hpp>

#include <opm/output/data/RegionsetVariableDescriptor.hpp>
#include <opm/output/data/RegionVariableMapping.hpp>
#include <opm/output/data/RegionVariableValues.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <memory>
#include <optional>

namespace {

    Opm::EclipseState staticProperties()
    {
        return Opm::EclipseState {
            Opm::Parser{}.parseString(R"(RUNSPEC
DIMENS
1 5 2 /
OIL
GAS
WATER
TABDIMS
/

GRID
DXV
100 /
DYV
5*100 /
DZV
2*10 /
DEPTHZ
12*2000 /
EQUALS
 PERMX 100 /
 PERMY 100 /
 PERMZ  10 /
 PORO    0.3 /
/

PROPS
DENSITY
 800 1000 1 /

REGIONS
FIPNUM
 5*1 5*2 /
FIPABC
 1 1 3 3 2
 1 1 3 3 2 /
END
)")
        };
    }

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Single_Regset)

namespace {

    Opm::data::RegionVariableMapping mapping(const std::string& regset = "FIPNUM")
    {
        auto varMap = Opm::data::RegionVariableMapping{};

        varMap.prepareRegistration();

        varMap.add(Opm::data::RegionVariableMapping::RegionSet { regset });

        return varMap;
    }

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Non_Cumulative)

BOOST_AUTO_TEST_SUITE(Single_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0 , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0 , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.23, 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * 1.23, 1.0e-8);

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.23, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0 , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0 , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 4.56, 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * 4.56, 1.0e-8);

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * 4.56, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 4.56, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * 4.56, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Single_Var

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Multi_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 4 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 4 * 2.2, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 4 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 4 * 3.3, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 4 * 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.1, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 2.2, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 2 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 3.3, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 2 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * 1.1, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6 * 2.2, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 3 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(1, 3), 2 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 6 * 3.3, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(1, 3), 2 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 6 * 4.4, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 3 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(1, 3), 2 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 4.4);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 4 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 4 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 4 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 4 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4 * 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 4 * 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 5.5, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 1 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 6.6, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 2 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 1 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 7.7, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 2 * 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 1 * 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 8.8, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * 5.5, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * 5.5, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 5.5, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * 5.5, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6 * 6.6, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 3 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 6.6, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(1, 3), 2 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 6 * 7.7, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 7.7, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(1, 3), 2 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 6 * 8.8, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 3 * 8.8, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 8.8, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(1, 3), 2 * 8.8, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Multi_Var

BOOST_AUTO_TEST_SUITE_END()     // Non_Cumulative

// ###########################################################################

BOOST_AUTO_TEST_SUITE(Cumulative)

BOOST_AUTO_TEST_SUITE(Single_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0 , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0 , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.23, 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * 1.23, 1.0e-8);

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.23, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 1.23 + 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0        , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 1.23 + 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0        , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * (1.23 + 4.56), 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0          , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0          , 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * (1.23 + 4.56), 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0          , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 2
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto v1 = coll.regionVariableValues().values(0);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * (1.23 + 4.56), 1.0e-8);

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0          , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * (1.23 + 4.56), 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * (1.23 + 4.56), 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * (1.23 + 4.56), 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Single_Var

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Multi_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 4 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 4 * 2.2, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 4 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 4 * 3.3, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 4 * 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.1, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 2.2, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 2 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 3.3, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 2 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * 1.1, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6 * 2.2, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 3 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(1, 3), 2 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 6 * 3.3, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(1, 3), 2 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 6 * 4.4, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 3 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(1, 3), 2 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 0.11 + 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 0.11 + 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0       , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 0.22 + 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 0.22 + 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 0.0       , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 0.33 + 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 0.33 + 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 0.0       , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 0.44 + 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 0.44 + 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 0.0       , 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0         , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 4 * (0.22 + 6.6), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 4 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2),     0.0         , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 4 * (0.33 + 7.7), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 4 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2),     0.0         , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 4 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2),     0.0         , 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2 * (0.22 + 6.6), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 1 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 2 * (0.33 + 7.7), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 1 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 2 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * (0.11 + 5.5), 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6 * (0.22 + 6.6), 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 3 * (0.22 + 6.6), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * (0.22 + 6.6), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(1, 3), 2 * (0.22 + 6.6), 1.0e-8); // FIPABC == 3
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 6 * (0.33 + 7.7), 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3 * (0.33 + 7.7), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * (0.33 + 7.7), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(1, 3), 2 * (0.33 + 7.7), 1.0e-8); // FIPABC == 3
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 6 * (0.44 + 8.8), 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 3 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(1, 3), 2 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Multi_Var

BOOST_AUTO_TEST_SUITE_END()     // Cumulative

// ###########################################################################

BOOST_AUTO_TEST_SUITE(Mix_Var_Type)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 4 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 4 * 2.2, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 4 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 4 * 3.3, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 4 * 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.1, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 2.2, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 2 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 3.3, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 2 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 4.4, 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * 1.1, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6 * 2.2, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 3 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(1, 3), 2 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 6 * 3.3, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(1, 3), 2 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 6 * 4.4, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 3 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(1, 3), 2 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 0.11 + 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 0.11 + 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 0.0       , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 0.44 + 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 0.44 + 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 0.0       , 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 4 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 4 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2),     0.0         , 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 4 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 4 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 4 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 4 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2),     0.0, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 4 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 4 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2),     0.0         , 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 2 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 2 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 1 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 6.6, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 2 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 1 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 7.7, 1.0e-8); // FIPNUM == 2
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 2 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 2
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping("FIPABC");
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    {
        const auto v1 = coll.regionVariableValues().values(0);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(0, 0), 6 * (0.11 + 5.5), 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(1, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(1, 1), 3 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(1, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(1, 3), 2 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto v2 = coll.regionVariableValues().values(1);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(0, 0), 6 * 6.6, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(1, 1), 3 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(1, 2), 1 * 6.6, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(1, 3), 2 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v3 = coll.regionVariableValues().values(2);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(0, 0), 6 * 7.7, 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(1, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(1, 1), 3 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(1, 2), 1 * 7.7, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(1, 3), 2 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto v4 = coll.regionVariableValues().values(3);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(0, 0), 6 * (0.44 + 8.8), 1.0e-8);

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(1, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(1, 1), 3 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(1, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(1, 3), 2 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Mix_Var_Type

BOOST_AUTO_TEST_SUITE_END()     // Single_Regset

// ///////////////////////////////////////////////////////////////////////////
// ###########################################################################
// ///////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_SUITE(Two_Regsets)

namespace {

    Opm::data::RegionVariableMapping mapping()
    {
        auto varMap = Opm::data::RegionVariableMapping{};

        varMap.prepareRegistration();

        varMap.add(Opm::data::RegionVariableMapping::RegionSet { "FIPNUM" });
        varMap.add(Opm::data::RegionVariableMapping::RegionSet { "FIPABC" });

        return varMap;
    }

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Non_Cumulative)

BOOST_AUTO_TEST_SUITE(Single_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0 , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0 , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0 , 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0 , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0     , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.23, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * 1.23, 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0     , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0     , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * 1.23, 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.23, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0 , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 4.56, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0 , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0 , 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0 , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * 4.56, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 4.56, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 4.56, 1.0e-8); // FIPABC == 2
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * 4.56, 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * 4.56, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0 , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 4.56, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * 4.56, 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * 4.56, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 4.56, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * 4.56, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Single_Var

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Multi_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 0.0, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 0.0, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 4 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 4 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 4 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 4 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 2 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 4 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 2 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * 1.1, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 1 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 1 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 2 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 1 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 1 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 2 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 1 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 1 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * 1.1, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 5 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 3 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 2 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 6 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 5 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 3 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 2 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 6 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 5 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 3 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 2 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 5.5, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 8.8, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 4.4);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * 5.5, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 5.5, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 5.5, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 4 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 4 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 6.6, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 4 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 4 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 2 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 7.7, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4 * 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 4 * 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 2 * 8.8, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 8.8, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 8.8, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * 5.5, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * 5.5, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 5.5, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 1 * 6.6, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 1 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 2 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 1 * 7.7, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 1 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 2 * 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 1 * 8.8, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 1 * 8.8, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 8.8, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * 5.5, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * 5.5, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 5.5, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * 5.5, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 5 * 6.6, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 3 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 6.6, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 2 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 6 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 5 * 7.7, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 3 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 7.7, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 2 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 6 * 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 5 * 8.8, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 3 * 8.8, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 8.8, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 2 * 8.8, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Multi_Var

BOOST_AUTO_TEST_SUITE_END()     // Non_Cumulative

// ###########################################################################

BOOST_AUTO_TEST_SUITE(Cumulative)

BOOST_AUTO_TEST_SUITE(Single_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0 , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0 , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0 , 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0 , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.23, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * 1.23, 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0 , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    coll.prepareValueAccumulation();

    const auto& grid = es.getInputGrid();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * 1.23, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0 , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.23, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * 1.23, 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0 , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * 1.23, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.23, 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * 1.23, 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 1.23 + 4.56, 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0        , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1.23 + 4.56, 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0        , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0        , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1.23 + 4.56, 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0        , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0        , 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * (1.23 + 4.56), 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0          , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0          , 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0          , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * (1.23 + 4.56), 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * (1.23 + 4.56), 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * (1.23 + 4.56), 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * (1.23 + 4.56), 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0          , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0          , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * (1.23 + 4.56), 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0          , 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * (1.23 + 4.56), 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.23);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.23);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.23);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.23);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.56);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.56);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.56);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.56);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    const auto iV1 = coll.variableIndex(varMap, "V1");

    BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

    const auto v1 = coll.regionVariableValues().values(*iV1);

    BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

    // FIELD
    BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * (1.23 + 4.56), 1.0e-8);

    // FIPNUM
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0          , 1.0e-8); // FIPNUM == 0
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 1
    BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * (1.23 + 4.56), 1.0e-8); // FIPNUM == 2

    // FIPABC
    BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0          , 1.0e-8); // FIPABC == 0
    BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * (1.23 + 4.56), 1.0e-8); // FIPABC == 1
    BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * (1.23 + 4.56), 1.0e-8); // FIPABC == 2
    BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * (1.23 + 4.56), 1.0e-8); // FIPABC == 3
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Single_Var

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Multi_Var)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 0.0, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 0.0, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 4 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 4 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 4 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 4 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 2 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 4 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 2 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * 1.1, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 1 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 1 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 2 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 1 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 1 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 2 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 1 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 1 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * 1.1, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 5 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 3 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 2 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 6 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 5 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 3 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 2 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 6 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 5 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 3 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 2 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 0.11 + 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 0.11 + 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0       , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0       , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 0.11 + 5.5, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0       , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0       , 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 0.22 + 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 0.22 + 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 0.0       , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0), 0.0       , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 0.22 + 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 0.0       , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 0.0       , 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 0.33 + 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 0.33 + 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 0.0       , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0), 0.0       , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 0.33 + 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 0.0       , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 0.0       , 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 0.44 + 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 0.44 + 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 0.0       , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0), 0.0       , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 0.44 + 8.8, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 0.0       , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 0.0       , 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0         , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 4 * (0.22 + 6.6), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 4 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2),     0.0         , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2 * (0.22 + 6.6), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * (0.22 + 6.6), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * (0.22 + 6.6), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 4 * (0.33 + 7.7), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 4 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2),     0.0         , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 2 * (0.33 + 7.7), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * (0.33 + 7.7), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * (0.33 + 7.7), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 4 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2),     0.0         , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 2 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0         , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2 * (0.22 + 6.6), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 1 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 1 * (0.22 + 6.6), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2),     0.0         , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * (0.22 + 6.6), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 2 * (0.33 + 7.7), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 1 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 1 * (0.33 + 7.7), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2),     0.0         , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * (0.33 + 7.7), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 2 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2),     0.0         , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6 * (0.22 + 6.6), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 5 * (0.22 + 6.6), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 3 * (0.22 + 6.6), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * (0.22 + 6.6), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 2 * (0.22 + 6.6), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 6 * (0.33 + 7.7), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 5 * (0.33 + 7.7), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 3 * (0.33 + 7.7), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * (0.33 + 7.7), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 2 * (0.33 + 7.7), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 6 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 5 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 3 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 2 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Multi_Var

BOOST_AUTO_TEST_SUITE_END()     // Cumulative

// ###########################################################################

BOOST_AUTO_TEST_SUITE(Mix_Var_Type)

BOOST_AUTO_TEST_SUITE(Single_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 0.0, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 0.0, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // -----------------------------------------------------------------

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 4 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 4 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 4 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 4 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 2 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1),     0.0, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 4 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 2 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * 1.1, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 1 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 1 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 2 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 1 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 1 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 2 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 1 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 1 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ false);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 1.1);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 1.1);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 1.1);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 1.1);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 2.2);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 2.2);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 2.2);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 2.2);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 3.3);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 3.3);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 3.3);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 3.3);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 4.4);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 4.4);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 4.4);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 4.4);

    coll.commitValues();

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * 1.1, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * 1.1, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * 1.1, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * 1.1, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * 1.1, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * 1.1, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6 * 2.2, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 2.2, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 5 * 2.2, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 3 * 2.2, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 2.2, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 2 * 2.2, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 6 * 3.3, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 3.3, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 5 * 3.3, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 3 * 3.3, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 3.3, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 2 * 3.3, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 6 * 4.4, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * 4.4, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 5 * 4.4, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 3 * 4.4, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * 4.4, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 2 * 4.4, 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Accumulation

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multi_Accumulation)

BOOST_AUTO_TEST_CASE(Single_Assign)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 0.11 + 5.5, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 0.11 + 5.5, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 0.0       , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0), 0.0       , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 0.11 + 5.5, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 0.0       , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 0.0       , 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0), 0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0), 0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 0.0, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 0.44 + 8.8, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0), 0.0       , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 0.44 + 8.8, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 0.0       , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0), 0.0       , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 0.44 + 8.8, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 0.0       , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 0.0       , 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Single_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 0),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 4 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 4 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2),     0.0         , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 2 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 4 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 4 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 2 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 6.6, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 4 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 4 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2),     0.0, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 2 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 7.7, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 4 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 4 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2),     0.0         , 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 2 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Single_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 2, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 2 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2),     0.0         , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 2 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 1 * 6.6, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 1 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 1 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 2 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 1 * 7.7, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 1 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2),     0.0, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 1 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 2 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2),     0.0         , 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_CASE(Multi_Assign_Multi_Reg)
{
    // Recall: Lifetime of FieldPropsManager must exceed the "variable collection".
    const auto es = staticProperties();

    auto coll = Opm::RegionVariableCollection {
        std::make_unique<Opm::data::RegionsetVariableDescriptor>(),
        std::make_unique<Opm::data::RegionVariableValues>()
    };

    auto varMap = mapping();
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V1" }, /* cumulative = */ true);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V2" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V3" }, /* cumulative = */ false);
    varMap.add(Opm::data::RegionVariableMapping::Variable { "V4" }, /* cumulative = */ true);

    varMap.commitStructure();

    coll.initialise(/* declaredMaxID = */ 0, es.fieldProps(), varMap);

    const auto& grid = es.getInputGrid();

    // -----------------------------------------------------------------

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.11);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.11);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.11);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.11);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.22);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.22);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.22);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.22);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.33);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.33);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.33);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.33);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 0.44);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 0.44);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 0.44);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 0.44);

    coll.commitValues();

    // =================================================================

    coll.prepareValueAccumulation();

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 5.5);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 5.5);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 5.5);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 0,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 5.5);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 6.6);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 6.6);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 6.6);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 1,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 6.6);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 7.7);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 7.7);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 7.7);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 2,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 7.7);

    // -----------------------------------------------------------------

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 0),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 2, 1),
                      /* x       = */ 8.8);

    // FIPABC = 2
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 4, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 1, 1),
                      /* x       = */ 8.8);

    // FIPABC = 1
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 0, 1),
                      /* x       = */ 8.8);

    // FIPABC = 3
    coll.addCellValue(/* var_ix  = */ 3,
                      /* cell_ix = */ grid.activeIndex(0, 3, 1),
                      /* x       = */ 8.8);

    coll.commitValues();

    // -----------------------------------------------------------------

    const auto iFLD = coll.regionSetIndex(varMap, "FIELD");
    const auto iNUM = coll.regionSetIndex(varMap, "FIPNUM");
    const auto iABC = coll.regionSetIndex(varMap, "FIPABC");

    BOOST_REQUIRE_MESSAGE(iFLD.has_value(), R"(Pseudo-region set "FIELD" must have an index)");
    BOOST_REQUIRE_MESSAGE(iNUM.has_value(), R"(Region set "FIPNUM" must have an index)");
    BOOST_REQUIRE_MESSAGE(iABC.has_value(), R"(Region set "FIPABC" must have an index)");

    {
        const auto iV1 = coll.variableIndex(varMap, "V1");

        BOOST_REQUIRE_MESSAGE(iV1.has_value(), R"(Variable "V1" must have an index)");

        const auto v1 = coll.regionVariableValues().values(*iV1);

        BOOST_REQUIRE_MESSAGE(v1.has_value(), R"(Variable "V1" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v1->element(*iFLD, 0), 6 * (0.11 + 5.5), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 1), 1 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v1->element(*iNUM, 2), 5 * (0.11 + 5.5), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v1->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v1->element(*iABC, 1), 3 * (0.11 + 5.5), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v1->element(*iABC, 2), 1 * (0.11 + 5.5), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v1->element(*iABC, 3), 2 * (0.11 + 5.5), 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV2 = coll.variableIndex(varMap, "V2");

        BOOST_REQUIRE_MESSAGE(iV2.has_value(), R"(Variable "V2" must have an index)");

        const auto v2 = coll.regionVariableValues().values(*iV2);

        BOOST_REQUIRE_MESSAGE(v2.has_value(), R"(Variable "V2" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v2->element(*iFLD, 0), 6 * 6.6, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 1), 1 * 6.6, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v2->element(*iNUM, 2), 5 * 6.6, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v2->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v2->element(*iABC, 1), 3 * 6.6, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v2->element(*iABC, 2), 1 * 6.6, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v2->element(*iABC, 3), 2 * 6.6, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV3 = coll.variableIndex(varMap, "V3");

        BOOST_REQUIRE_MESSAGE(iV3.has_value(), R"(Variable "V3" must have an index)");

        const auto v3 = coll.regionVariableValues().values(*iV3);

        BOOST_REQUIRE_MESSAGE(v3.has_value(), R"(Variable "V3" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v3->element(*iFLD, 0), 6 * 7.7, 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 0),     0.0, 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 1), 1 * 7.7, 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v3->element(*iNUM, 2), 5 * 7.7, 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v3->element(*iABC, 0),     0.0, 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v3->element(*iABC, 1), 3 * 7.7, 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v3->element(*iABC, 2), 1 * 7.7, 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v3->element(*iABC, 3), 2 * 7.7, 1.0e-8); // FIPABC == 3
    }

    {
        const auto iV4 = coll.variableIndex(varMap, "V4");

        BOOST_REQUIRE_MESSAGE(iV4.has_value(), R"(Variable "V4" must have an index)");

        const auto v4 = coll.regionVariableValues().values(*iV4);

        BOOST_REQUIRE_MESSAGE(v4.has_value(), R"(Variable "V4" must have defined values)");

        // FIELD
        BOOST_CHECK_CLOSE(v4->element(*iFLD, 0), 6 * (0.44 + 8.8), 1.0e-8);

        // FIPNUM
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 0),     0.0         , 1.0e-8); // FIPNUM == 0
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 1), 1 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 1
        BOOST_CHECK_CLOSE(v4->element(*iNUM, 2), 5 * (0.44 + 8.8), 1.0e-8); // FIPNUM == 2

        // FIPABC
        BOOST_CHECK_CLOSE(v4->element(*iABC, 0),     0.0         , 1.0e-8); // FIPABC == 0
        BOOST_CHECK_CLOSE(v4->element(*iABC, 1), 3 * (0.44 + 8.8), 1.0e-8); // FIPABC == 1
        BOOST_CHECK_CLOSE(v4->element(*iABC, 2), 1 * (0.44 + 8.8), 1.0e-8); // FIPABC == 2
        BOOST_CHECK_CLOSE(v4->element(*iABC, 3), 2 * (0.44 + 8.8), 1.0e-8); // FIPABC == 3
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Multi_Accumulation

BOOST_AUTO_TEST_SUITE_END()     // Mix_Var_Type

BOOST_AUTO_TEST_SUITE_END()     // Two_Regsets
