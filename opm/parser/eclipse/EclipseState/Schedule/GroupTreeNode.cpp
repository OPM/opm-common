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

#include <opm/parser/eclipse/EclipseState/Schedule/GroupTreeNode.hpp>
#include <stdexcept>

namespace Opm {

    GroupTreeNode::GroupTreeNode(const std::string& name) {
        m_name = name;
        m_parent = NULL;
    }

    GroupTreeNode::GroupTreeNode(const std::string& name, GroupTreeNode * parent) {
        m_name = name;
        m_parent = parent;
    }

    const std::string& GroupTreeNode::name() const {
        return m_name;
    }

    GroupTreeNode * GroupTreeNode::parent() const {
        return m_parent;
    }

    GroupTreeNodePtr GroupTreeNode::addChildGroup(const std::string& childName) {
        if (hasChildGroup(childName)) {
            throw std::invalid_argument("Child group with name \"" + childName + "\"already exists.");
        }
        GroupTreeNodePtr child(new GroupTreeNode(childName, this));
        m_childGroups[childName] = child;
        return child;
    }

    bool GroupTreeNode::hasChildGroup(const std::string& childName) const {
        return m_childGroups.find(childName) != m_childGroups.end();
    }

    GroupTreeNodeConstPtr GroupTreeNode::getChildGroup(const std::string& childName) {
        if (hasChildGroup(childName)) {
            return m_childGroups[childName];
        }
        throw std::invalid_argument("Child group with name \"" + childName + "\" does not exist.");
    }

    GroupTreeNodePtr GroupTreeNode::createFieldNode() {
        return GroupTreeNodePtr(new GroupTreeNode("FIELD"));
    }

}