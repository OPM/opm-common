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

#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>

#define BOOST_TEST_MODULE GroupTreeTests

#include <boost/test/unit_test.hpp>

#include <stdexcept>
#include <iostream>

using namespace Opm;

BOOST_AUTO_TEST_CASE(CreateGroupTree_DefaultConstructor_HasFieldNode) {
    GroupTree tree;
    BOOST_CHECK(tree.exists("FIELD"));
}

BOOST_AUTO_TEST_CASE(GetNode_NonExistingNode_ReturnsNull) {
    GroupTree tree;
    BOOST_CHECK(!tree.exists("Non-existing"));
}

BOOST_AUTO_TEST_CASE(GetNodeAndParent_AllOK) {
    GroupTree tree;
    tree.update("GRANDPARENT", "FIELD");
    tree.update("PARENT", "GRANDPARENT");
    tree.update("GRANDCHILD", "PARENT");

    const auto grandchild = tree.exists("GRANDCHILD");
    BOOST_CHECK(grandchild);
    auto parent = tree.parent("GRANDCHILD");
    BOOST_CHECK_EQUAL("PARENT", parent);
    BOOST_CHECK( tree.children( "PARENT" ).front() == "GRANDCHILD" );
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentNotSpecified_AddedUnderField) {
    GroupTree tree;
    tree.update("CHILD_OF_FIELD");
    BOOST_CHECK(tree.exists("CHILD_OF_FIELD"));
    BOOST_CHECK_EQUAL( "FIELD", tree.parent( "CHILD_OF_FIELD" ) );
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentIsField_AddedUnderField) {
    GroupTree tree;
    tree.update("CHILD_OF_FIELD", "FIELD");
    BOOST_CHECK( tree.exists( "CHILD_OF_FIELD" ) );
    BOOST_CHECK_EQUAL( "FIELD", tree.parent( "CHILD_OF_FIELD" ) );
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentNotAdded_ChildAndParentAdded) {
    GroupTree tree;
    tree.update("CHILD", "NEWPARENT");
    BOOST_CHECK( tree.exists("CHILD") );
    BOOST_CHECK_EQUAL( "NEWPARENT", tree.parent( "CHILD" ) );
    BOOST_CHECK_EQUAL( "CHILD", tree.children( "NEWPARENT" ).front() );
}

BOOST_AUTO_TEST_CASE(UpdateTree_AddFieldNode_Throws) {
    GroupTree tree;
    BOOST_CHECK_THROW(tree.update("FIELD", "NEWPARENT"), std::invalid_argument );
    BOOST_CHECK_THROW(tree.update("FIELD"), std::invalid_argument );
}
