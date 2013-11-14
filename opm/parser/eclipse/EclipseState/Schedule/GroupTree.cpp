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

    GroupTreeNodePtr GroupTree::updateTree(const std::string& childName, const std::string& parentName ) {
        if (getNode(childName)) {
            throw std::invalid_argument("Node \"" + childName + "\" is already in tree, cannot move (yet)");
        }
        GroupTreeNodePtr parentNode = getNode(parentName);
        if (!parentNode){
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
                if (getNode(nodeName, (*iter).second)) {
                    return (*iter).second;
                }
                ++iter;
            }
            return GroupTreeNodePtr();
        }
    }

}