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


using namespace Opm;

BOOST_AUTO_TEST_CASE(CreateGroupTree_DefaultConstructor_HasFieldNode) {
    GroupTree tree;
    BOOST_CHECK(tree.getNode("FIELD"));
}

BOOST_AUTO_TEST_CASE(GetNode_NonExistingNode_ReturnsNull) {
    GroupTree tree;
    BOOST_CHECK_EQUAL(GroupTreeNodePtr(), tree.getNode("Non-existing"));
}

BOOST_AUTO_TEST_CASE(AddNode_ParentNotSpecified_AddedUnderField) {
    GroupTree tree;
    tree.updateTree("CHILD_OF_FIELD");
    BOOST_CHECK(tree.getNode("CHILD_OF_FIELD"));
    GroupTreeNodePtr rootNode = tree.getNode("FIELD");
    BOOST_CHECK(rootNode->hasChildGroup("CHILD_OF_FIELD"));
}

BOOST_AUTO_TEST_CASE(AddNode_ParentIsField_AddedUnderField) {
    GroupTree tree;
    tree.updateTree("CHILD_OF_FIELD", "FIELD");
    BOOST_CHECK(tree.getNode("CHILD_OF_FIELD"));
    GroupTreeNodePtr rootNode = tree.getNode("FIELD");
    BOOST_CHECK(rootNode->hasChildGroup("CHILD_OF_FIELD"));
}


BOOST_AUTO_TEST_CASE(AddNode_ParentNotAdded_ChildAndParentAdded) {
    GroupTree tree;
    tree.updateTree("CHILD", "NEWPARENT");
    BOOST_CHECK(tree.getNode("CHILD"));
    GroupTreeNodePtr rootNode = tree.getNode("FIELD");
    BOOST_CHECK(rootNode->hasChildGroup("NEWPARENT"));
    GroupTreeNodePtr newParent = tree.getNode("NEWPARENT");
    BOOST_CHECK(newParent->hasChildGroup("CHILD"));
}

