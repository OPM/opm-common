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

#define BOOST_TEST_MODULE data_RegionsetVariableDescriptor

#include <boost/test/unit_test.hpp>

#include <opm/output/data/RegionsetVariableDescriptor.hpp>

#include <array>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <stdexcept>
#include <vector>

BOOST_AUTO_TEST_SUITE(Basic_Operations)

BOOST_AUTO_TEST_CASE(Empty)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{0});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{0});
}

BOOST_AUTO_TEST_SUITE(Single_Regset)

BOOST_AUTO_TEST_CASE(Single_Region)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* maxRegionID = */ 5); // Supports regions 0..5

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{6});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Single_Region_Scan_Regions)
{
    const auto regions = std::vector<int>(123, 0);

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 0, regions);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Single_Region_Scan_Regions_List)
{
    const auto regions = std::forward_list {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 0, regions);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Single_Region_Scan_Regions_Deque)
{
    const auto regions = std::deque {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 0, regions);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions_Scan_Regions_I)
{
    const auto regions = std::vector{ 1, 1, 2, 2, 1, 1, 3, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 5, regions);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{6});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions_Scan_Regions_II)
{
    const auto regions = std::vector{ 1, 1, 2, 2, 1, 1, 3, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 2, regions);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{4});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions_Scan_Regions_III)
{
    const auto regions = std::vector{ 1, 1, 2, 2, 1, 1, 3, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 3, regions);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{1});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{4});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Regset

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Two_Regsets)

BOOST_AUTO_TEST_CASE(Single_Region)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.
    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{1});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* maxRegionID = */ 5); // Supports regions 0..5
    d.addRegionSet(/* maxRegionID = */ 3); // Supports regions 0..3

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{10});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{6});
}

BOOST_AUTO_TEST_CASE(Single_Region_Scan_Regions)
{
    const auto reg_1 = std::vector { 0, 0, 0, 0, 0, };
    const auto reg_2 = std::deque { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 0, reg_1);
    d.addRegionSet(/* declaredMaxRegionID = */ 0, reg_2);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{1});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions_Scan_Regions_I)
{
    const auto reg_1 = std::vector { 1, 1, 2, 2, 1, 1, 3, };
    const auto reg_2 = std::array { 1, 1, 2, 2, 1, 1, 3, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 5, reg_1);
    d.addRegionSet(/* declaredMaxRegionID = */ 3, reg_2);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{10});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{6});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions_Scan_Regions_II)
{
    const auto reg_1 = std::vector { 1, 1, 2, 2, 1, 1, 3, };
    const auto reg_2 = std::array { 1, 1, 2, 2, 1, 1, 3, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 3, reg_1);
    d.addRegionSet(/* declaredMaxRegionID = */ 5, reg_2);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{10});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{4});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions_Scan_Regions_III)
{
    const auto reg_1 = std::vector { 1, 1, 2, 2, 1, 1, 3, };
    const auto reg_2 = std::array { 1, 1, 2, 2, 1, 1, 3, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 1, reg_1);
    d.addRegionSet(/* declaredMaxRegionID = */ 1, reg_2);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{8});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{4});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions_Scan_Regions_IV)
{
    const auto reg_1 = std::vector { 1, 1, 2, 2, 1, 1, 3, };
    const auto reg_2 = std::array { 1, 1, 2, 2, 1, 1, 3, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */ 17, reg_1);
    d.addRegionSet(/* declaredMaxRegionID = */ 29, reg_2);

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{2});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{48});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{18});
}

BOOST_AUTO_TEST_SUITE_END()     // Two_Regsets

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Multiple_Regsets)

BOOST_AUTO_TEST_CASE(Single_Region)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.
    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.
    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.
    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.
    d.addRegionSet(/* maxRegionID = */ 0); // "FIELD" or similar.

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{5});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{5});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{1});
    BOOST_CHECK_EQUAL(d.startIndex(2), std::size_t{2});
    BOOST_CHECK_EQUAL(d.startIndex(3), std::size_t{3});
    BOOST_CHECK_EQUAL(d.startIndex(4), std::size_t{4});
}

BOOST_AUTO_TEST_CASE(Multiple_Regions)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* maxRegionID = */ 4); // Supports regions 0..4
    d.addRegionSet(/* maxRegionID = */ 3); // Supports regions 0..3
    d.addRegionSet(/* maxRegionID = */ 2); // Supports regions 0..2
    d.addRegionSet(/* maxRegionID = */ 1); // Supports regions 0..1
    d.addRegionSet(/* maxRegionID = */ 0); // Supports regions 0..0

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{5});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{15});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{ 0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{ 5});
    BOOST_CHECK_EQUAL(d.startIndex(2), std::size_t{ 9});
    BOOST_CHECK_EQUAL(d.startIndex(3), std::size_t{12});
    BOOST_CHECK_EQUAL(d.startIndex(4), std::size_t{14});
}

BOOST_AUTO_TEST_CASE(Scan_Regions)
{
    const auto reg_1 = std::vector<int> {};
    const auto reg_2 = std::vector { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, };
    const auto reg_3 = std::vector { 3, 14, 1, 5, 9, 26, };
    const auto reg_4 = std::forward_list { 0, 0, 0, 0, 0, 0, };
    const auto reg_5 = std::deque { 11, 22, 33, 17, 29, };
    const auto reg_6 = std::array { 0, 1, 0, 2, 3, 0, 1, };

    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();

    d.addRegionSet(/* declaredMaxRegionID = */  5, reg_1); // max ID =  5
    d.addRegionSet(/* declaredMaxRegionID = */ 42, reg_2); // max ID = 42
    d.addRegionSet(/* declaredMaxRegionID = */  0, reg_3); // max ID = 26
    d.addRegionSet(/* declaredMaxRegionID = */  0, reg_4); // max ID =  0
    d.addRegionSet(/* declaredMaxRegionID = */ 11, reg_5); // max ID = 33
    d.addRegionSet(/* declaredMaxRegionID = */  5, reg_6); // max ID =  5

    d.finaliseDescriptorSet();

    BOOST_CHECK_EQUAL(d.numRegionSets(), std::size_t{6});
    BOOST_CHECK_EQUAL(d.numVariableSlots(), std::size_t{117});
    BOOST_CHECK_EQUAL(d.startIndex(0), std::size_t{  0});
    BOOST_CHECK_EQUAL(d.startIndex(1), std::size_t{  6});
    BOOST_CHECK_EQUAL(d.startIndex(2), std::size_t{ 49});
    BOOST_CHECK_EQUAL(d.startIndex(3), std::size_t{ 76});
    BOOST_CHECK_EQUAL(d.startIndex(4), std::size_t{ 77});
    BOOST_CHECK_EQUAL(d.startIndex(5), std::size_t{111});
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()     // Basic_Operations

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Erroneous_Accesses)

BOOST_AUTO_TEST_CASE(Add_Before_Prepare)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    BOOST_CHECK_THROW(d.addRegionSet(/* maxRegionID = */ 0), std::logic_error);
}

BOOST_AUTO_TEST_CASE(Finalise_Before_Prepare)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    BOOST_CHECK_THROW(d.finaliseDescriptorSet(), std::logic_error);
}

BOOST_AUTO_TEST_CASE(Add_After_Finalise)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();
    d.addRegionSet(/* maxRegionID = */ 0);
    d.finaliseDescriptorSet();

    BOOST_CHECK_THROW(d.addRegionSet(/* maxRegionID = */ 0), std::logic_error);
}

BOOST_AUTO_TEST_CASE(StartIndex_Before_Finalise)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();
    d.addRegionSet(/* maxRegionID = */ 0);

    BOOST_CHECK_THROW(d.startIndex(0), std::logic_error);
}

BOOST_AUTO_TEST_CASE(StartIndex_Out_Of_Range)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();
    d.addRegionSet(/* maxRegionID = */ 0);
    d.finaliseDescriptorSet();

    BOOST_CHECK_THROW(d.startIndex(1), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(StartIndex_Empty_Descriptor_Out_Of_Range)
{
    auto d = Opm::data::RegionsetVariableDescriptor{};

    d.prepareDescriptorSet();
    d.finaliseDescriptorSet();

    BOOST_CHECK_THROW(d.startIndex(0), std::out_of_range);
}

BOOST_AUTO_TEST_SUITE_END()     // Erroneous_Accesses
