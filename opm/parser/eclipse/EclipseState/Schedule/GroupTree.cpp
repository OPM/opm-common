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
        return updateTree(childName, m_root->name());
    }

    GroupTreeNodePtr GroupTree::updateTree(const std::string& childName, const std::string& parentName) {
        if (childName == m_root->name()) {
            throw std::domain_error("Error, trying to add node with the same name as the root, offending name: " + childName);
        }

        GroupTreeNodePtr newParentNode = getNode(parentName);
        if (!newParentNode) {
            newParentNode = updateTree(parentName);
        }

        GroupTreeNodePtr childNodeInTree = getNode(childName);
        if (childNodeInTree) {
            GroupTreeNodePtr currentParent = getParent(childName);
            currentParent->removeChild(childNodeInTree);
            newParentNode->addChildGroup(childNodeInTree);
            return childNodeInTree;
        } else {
            return newParentNode->addChildGroup(childName);
        }
    }

    GroupTreeNodePtr GroupTree::getNode(const std::string& nodeName) const {
        GroupTreeNodePtr current = m_root;
        return getNode(nodeName, current);
    }

    GroupTreeNodePtr GroupTree::getNode(const std::string& nodeName, GroupTreeNodePtr current) const {
        if (current->name() == nodeName) {
            return current;
        } else {
            std::map<std::string, GroupTreeNodePtr>::iterator iter = current->begin();
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

    GroupTreeNodePtr GroupTree::getParent(const std::string& childName) const {
        GroupTreeNodePtr currentChild = m_root;
        return getParent(childName, currentChild, GroupTreeNodePtr());
    }

    GroupTreeNodePtr GroupTree::getParent(const std::string& childName, GroupTreeNodePtr currentChild, GroupTreeNodePtr parent) const {
        if (currentChild->name() == childName) {
            return parent;
        } else {
            std::map<std::string, GroupTreeNodePtr>::iterator iter = currentChild->begin();
            while (iter != currentChild->end()) {
                GroupTreeNodePtr result = getParent(childName, (*iter).second, currentChild);
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
        std::map<std::string, GroupTreeNodePtr >::iterator iter = origin->begin();
        while (iter != origin->end()) {
            GroupTreeNodePtr originChild = (*iter).second;
            GroupTreeNodePtr copyChild = copy->addChildGroup(originChild->name());
            deepCopy(originChild, copyChild);
            ++iter;
        }
    }

    void GroupTree::printTree() const {
        printTree(m_root);
        std::cout << std::endl;
        std::cout << "End of tree" << std::endl;
    }

    void GroupTree::printTree(GroupTreeNodePtr fromNode) const {
        std::cout << fromNode->name() << "(" << fromNode.get() << ")";
        std::map<std::string, GroupTreeNodePtr >::iterator iter = fromNode->begin();
        while (iter != fromNode->end()) {
            GroupTreeNodePtr child = (*iter).second;
            std::cout << "<-" << child->name() << "(" << child.get() << ")" << std::endl;
            printTree(child);
            ++iter;
        }
    }

}
