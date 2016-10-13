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
    BOOST_CHECK(tree.getNode("FIELD"));
}

BOOST_AUTO_TEST_CASE(GetNode_NonExistingNode_ReturnsNull) {
    GroupTree tree;
    BOOST_CHECK(!tree.getNode("Non-existing"));
}

BOOST_AUTO_TEST_CASE(GetNodeAndParent_AllOK) {
    GroupTree tree;
    tree.updateTree("GRANDPARENT", "FIELD");
    tree.updateTree("PARENT", "GRANDPARENT");
    tree.updateTree("GRANDCHILD", "PARENT");

    auto grandchild = tree.getNode("GRANDCHILD");
    BOOST_CHECK(grandchild);
    auto parent = tree.getParent("GRANDCHILD");
    BOOST_CHECK_EQUAL("PARENT", parent->name());
    BOOST_CHECK(parent->hasChildGroup("GRANDCHILD"));
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentNotSpecified_AddedUnderField) {
    GroupTree tree;
    tree.updateTree("CHILD_OF_FIELD");
    BOOST_CHECK(tree.getNode("CHILD_OF_FIELD"));
    auto rootNode = tree.getNode("FIELD");
    BOOST_CHECK(rootNode->hasChildGroup("CHILD_OF_FIELD"));
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentIsField_AddedUnderField) {
    GroupTree tree;
    tree.updateTree("CHILD_OF_FIELD", "FIELD");
    BOOST_CHECK(tree.getNode("CHILD_OF_FIELD"));
    auto rootNode = tree.getNode("FIELD");
    BOOST_CHECK(rootNode->hasChildGroup("CHILD_OF_FIELD"));
}

BOOST_AUTO_TEST_CASE(UpdateTree_ParentNotAdded_ChildAndParentAdded) {
    GroupTree tree;
    tree.updateTree("CHILD", "NEWPARENT");
    BOOST_CHECK(tree.getNode("CHILD"));
    auto rootNode = tree.getNode("FIELD");
    BOOST_CHECK(rootNode->hasChildGroup("NEWPARENT"));
    auto newParent = tree.getNode("NEWPARENT");
    BOOST_CHECK(newParent->hasChildGroup("CHILD"));
}

BOOST_AUTO_TEST_CASE(UpdateTree_AddFieldNode_Throws) {
    GroupTree tree;
    BOOST_CHECK_THROW(tree.updateTree("FIELD", "NEWPARENT"), std::domain_error);
    BOOST_CHECK_THROW(tree.updateTree("FIELD"), std::domain_error);
}

BOOST_AUTO_TEST_CASE(UpdateTree_ChildExists_ChildMoved) {
    GroupTree tree;
    tree.updateTree("OLDPARENT", "FIELD");
    tree.updateTree("NEWPARENT", "FIELD");
    tree.updateTree("THECHILD", "OLDPARENT");
    tree.updateTree("GRANDCHILD1", "THECHILD");
    tree.updateTree("GRANDCHILD2", "THECHILD");

    auto oldParent = tree.getNode("OLDPARENT");
    BOOST_CHECK(oldParent->hasChildGroup("THECHILD"));
    auto theChild = oldParent->getChildGroup("THECHILD");
    BOOST_CHECK(theChild->hasChildGroup("GRANDCHILD1"));

    auto newParent = tree.getNode("NEWPARENT");
    BOOST_CHECK(!newParent->hasChildGroup("THECHILD"));

    tree.updateTree("THECHILD", "NEWPARENT");

    BOOST_CHECK(!oldParent->hasChildGroup("THECHILD"));

    BOOST_CHECK(newParent->hasChildGroup("THECHILD"));
    theChild = newParent->getChildGroup("THECHILD");
    BOOST_CHECK(theChild->hasChildGroup("GRANDCHILD1"));
}

BOOST_AUTO_TEST_CASE(DeepCopy_TreeWithChildren_ObjectsDifferContentMatch) {
    GroupTree tree;
    tree.updateTree("L1CHILD1", "FIELD");
    tree.updateTree("L1CHILD2", "FIELD");
    tree.updateTree("L2CHILD1", "L1CHILD1");
    tree.updateTree("L2CHILD2", "L1CHILD1");
    tree.updateTree("L3CHILD1", "L2CHILD1");

    auto copiedTree = tree.deepCopy();
    auto fieldNodeCopy = copiedTree->getNode("FIELD");
    auto fieldNodeOriginal = tree.getNode("FIELD");
    BOOST_CHECK(!(fieldNodeCopy == fieldNodeOriginal));
    BOOST_CHECK_EQUAL(fieldNodeCopy->name(), fieldNodeOriginal->name());

    auto L1CHILD1NodeCopy = fieldNodeCopy->getChildGroup("L1CHILD1");
    auto L1CHILD1NodeOriginal = fieldNodeOriginal->getChildGroup("L1CHILD1");
    BOOST_CHECK(!(L1CHILD1NodeCopy == L1CHILD1NodeOriginal));
    BOOST_CHECK_EQUAL(L1CHILD1NodeCopy->name(), L1CHILD1NodeOriginal->name());

    auto L1CHILD2NodeCopy = fieldNodeCopy->getChildGroup("L1CHILD2");
    auto L1CHILD2NodeOriginal = fieldNodeOriginal->getChildGroup("L1CHILD2");
    BOOST_CHECK(!(L1CHILD2NodeCopy == L1CHILD2NodeOriginal));
    BOOST_CHECK_EQUAL(L1CHILD2NodeCopy->name(), L1CHILD2NodeOriginal->name());

    auto L2CHILD1NodeCopy = L1CHILD1NodeCopy->getChildGroup("L2CHILD1");
    auto L2CHILD1NodeOriginal = L1CHILD1NodeOriginal->getChildGroup("L2CHILD1");
    BOOST_CHECK(!(L2CHILD1NodeCopy == L2CHILD1NodeOriginal));
    BOOST_CHECK_EQUAL(L2CHILD1NodeCopy->name(), L2CHILD1NodeOriginal->name());

    auto L2CHILD2NodeCopy = L1CHILD1NodeCopy->getChildGroup("L2CHILD2");
    auto L2CHILD2NodeOriginal = L1CHILD1NodeOriginal->getChildGroup("L2CHILD2");
    BOOST_CHECK(!(L2CHILD2NodeCopy == L2CHILD2NodeOriginal));
    BOOST_CHECK_EQUAL(L2CHILD2NodeCopy->name(), L2CHILD2NodeOriginal->name());

    auto L3CHILD1NodeCopy = L2CHILD1NodeCopy->getChildGroup("L3CHILD1");
    auto L3CHILD1NodeOriginal = L2CHILD1NodeOriginal->getChildGroup("L3CHILD1");
    BOOST_CHECK(!(L3CHILD1NodeCopy == L3CHILD1NodeOriginal));
    BOOST_CHECK_EQUAL(L3CHILD1NodeCopy->name(), L3CHILD1NodeOriginal->name());
}

BOOST_AUTO_TEST_CASE(GetNodes_ReturnsAllNodes) {
    GroupTree tree;
    tree.updateTree("L1CHILD1", "FIELD");
    tree.updateTree("L1CHILD2", "FIELD");
    tree.updateTree("L2CHILD1", "L1CHILD1");
    tree.updateTree("L2CHILD2", "L1CHILD1");
    tree.updateTree("L3CHILD1", "L2CHILD1");

    const auto& nodes = tree.getNodes();
    BOOST_CHECK_EQUAL(6U, nodes.size());
    BOOST_CHECK_EQUAL("FIELD", nodes[0U]->name());
    BOOST_CHECK_EQUAL("L1CHILD1", nodes[1U]->name());
    BOOST_CHECK_EQUAL("L2CHILD1", nodes[2U]->name());
    BOOST_CHECK_EQUAL("L3CHILD1", nodes[3U]->name());
    BOOST_CHECK_EQUAL("L2CHILD2", nodes[4U]->name());
    BOOST_CHECK_EQUAL("L1CHILD2", nodes[5U]->name());
}
