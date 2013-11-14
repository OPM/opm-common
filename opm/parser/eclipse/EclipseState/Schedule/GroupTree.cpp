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

#include <stdexcept>
#include <iostream>


#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>


namespace Opm {

    GroupTree::GroupTree() {
        m_root = GroupTreeNode::createFieldNode();
    }

    GroupTreeNodePtr GroupTree::updateTree(const std::string& childName) {
        if (getNode(childName)) {
            throw std::invalid_argument("Node \"" + childName + "\" is already in tree, cannot move (yet)");
        }
        return m_root->addChildGroup(childName);
    }

    GroupTreeNodePtr GroupTree::updateTree(const std::string& childName, const std::string& parentName) {
        if (getNode(childName)) {
            throw std::invalid_argument("Node \"" + childName + "\" is already in tree, cannot move (yet)");
        }
        GroupTreeNodePtr parentNode = getNode(parentName);
        if (!parentNode) {
            parentNode = updateTree(parentName);
        }
        return parentNode->addChildGroup(childName);
    }

    GroupTreeNodePtr GroupTree::getNode(const std::string& nodeName) const {
        GroupTreeNodePtr current = m_root;
        return getNode(nodeName, current);
    }

    GroupTreeNodePtr GroupTree::getNode(const std::string& nodeName, GroupTreeNodePtr current) const {
        if (current->name() == nodeName) {
            return current;
        } else {
            auto iter = current->begin();
            while (iter != current->end()) {
                GroupTreeNodePtr result = getNode(nodeName, (*iter).second); 
                if (result) {
                    return result;
                }
                ++iter;
            }
            return GroupTreeNodePtr();
        }
    }

    GroupTreePtr GroupTree::deepCopy() const {
        GroupTreePtr newTree(new GroupTree());
        GroupTreeNodePtr currentOriginNode = m_root;
        GroupTreeNodePtr currentNewNode = newTree->getNode("FIELD");

        deepCopy(currentOriginNode, currentNewNode);
        return newTree;

    }

    void GroupTree::deepCopy(GroupTreeNodePtr origin, GroupTreeNodePtr copy) const {
        auto iter = origin->begin();
        while (iter != origin->end()) {
            GroupTreeNodePtr originChild = (*iter).second;
            GroupTreeNodePtr copyChild = copy->addChildGroup(originChild->name());
            deepCopy(originChild, copyChild);
            ++iter;
        }
    }

    void GroupTree::printTree() const {
        printTree(m_root);
    }

    void GroupTree::printTree(GroupTreeNodePtr fromNode) const {
        auto iter = fromNode->begin();
        while (iter != fromNode->end()) {
            
            GroupTreeNodePtr child = (*iter).second;
            std::cout << fromNode->name() << "(" << fromNode.get() << ")" <<  "<-" << child->name() << "(" << child.get() << ")" << std::endl;
            printTree(child);
            ++iter;
        }
    }

}