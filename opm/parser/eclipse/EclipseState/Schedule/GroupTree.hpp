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

#ifndef GROUPTREE_HPP
#define	GROUPTREE_HPP

#include <opm/parser/eclipse/EclipseState/Schedule/GroupTreeNode.hpp>

#include <string>
#include <map>
#include <memory>
#include <vector>

namespace Opm {

    class GroupTree {
    public:
        GroupTree();
        GroupTree( const GroupTree& );
        void updateTree(const std::string& childName);
        void updateTree(const std::string& childName, const std::string& parentName);

        std::shared_ptr< GroupTreeNode > getNode(const std::string& nodeName) const;
        std::vector<std::shared_ptr< const GroupTreeNode >> getNodes() const;
        std::shared_ptr< GroupTreeNode > getParent(const std::string& childName) const;

        std::shared_ptr<GroupTree> deepCopy() const;
        void printTree(std::ostream &os) const;

        bool operator==( const GroupTree& rhs ) {
            return this->m_root == rhs.m_root;
        }

        bool operator!=( const GroupTree& rhs ) {
            return !(*this == rhs );
        }


    private:
        std::shared_ptr< GroupTreeNode > m_root;
        std::shared_ptr< GroupTreeNode > getNode(const std::string& nodeName, std::shared_ptr< GroupTreeNode > current) const;
        std::shared_ptr< GroupTreeNode > getParent(const std::string& childName, std::shared_ptr< GroupTreeNode > currentChild, std::shared_ptr< GroupTreeNode > parent) const;

        void getNodes(std::shared_ptr< GroupTreeNode > fromNode,
                      std::vector< std::shared_ptr< const GroupTreeNode > >& nodes) const;
        void deepCopy(std::shared_ptr< GroupTreeNode > origin, std::shared_ptr< GroupTreeNode > copy) const;
        void printTree(std::ostream &os , std::shared_ptr< GroupTreeNode > fromNode) const;
    };
}

#endif	/* GROUPTREE_HPP */

