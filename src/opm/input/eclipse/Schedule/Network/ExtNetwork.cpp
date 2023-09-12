/*
  Copyright 2020 Equinor ASA.

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
#include <algorithm>
#include <iterator>
#include <stdexcept>

#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>

namespace Opm {
namespace Network {

ExtNetwork ExtNetwork::serializationTestObject() {
    ExtNetwork object;
    object.m_branches = {Branch::serializationTestObject()};
    object.insert_indexed_node_names = {"test1", "test2"};
    object.m_nodes = {{"test3", Node::serializationTestObject()}};
    return object;
}

bool ExtNetwork::active() const {
    return !this->m_branches.empty() && !this->m_nodes.empty();
}

bool ExtNetwork::operator==(const ExtNetwork& rhs) const {
    return this->m_branches == rhs.m_branches
        && this->insert_indexed_node_names == rhs.insert_indexed_node_names
        && this->m_nodes == rhs.m_nodes;
}


bool ExtNetwork::has_node(const std::string& name) const {
    return (this->m_nodes.count(name) > 0);
}

const Node& ExtNetwork::node(const std::string& name) const {
    const auto node_iter = this->m_nodes.find( name );
    if (node_iter == this->m_nodes.end())
        throw std::out_of_range("No such node: " + name);

    return node_iter->second;
}


const Node& ExtNetwork::root() const {
    if (this->m_nodes.empty())
        throw std::invalid_argument("No root defined for empty network");

    std::vector<const Node*> root;
    auto node_iter = this->m_nodes.begin();
    // Find all nodes with terminal pressure assigned to it
    while ((node_iter = std::find_if(node_iter, this->m_nodes.end(), [](const auto &n) { return (n.second.terminal_pressure().has_value() == true); })) != this->m_nodes.end()) {
        const auto node_ptr = &(node_iter->second);
        root.push_back(node_ptr);
        ++node_iter;
    }

    if (root.empty()) {
         throw std::invalid_argument("No root defined in the network");
    } else if (root.size() >= 2) {
         throw std::invalid_argument("Multiple roots in the network are not supported");
    }

    return *root[0];
}

void ExtNetwork::add_branch(Branch branch)
{
    for (const std::string& nodename : { branch.downtree_node(), branch.uptree_node() }) {
        if (!this->has_node(nodename)) {
            this->m_nodes.insert_or_assign(nodename, Node{nodename});
        }
        if (!this->has_indexed_node_name(nodename)) {
            this->add_indexed_node_name(nodename);
        }
    }
    this->m_branches.push_back( std::move(branch) );
}

void ExtNetwork::drop_branch(const std::string& uptree_node, const std::string& downtree_node) {
    auto downtree_branches = this->downtree_branches( downtree_node );
    if (downtree_branches.empty()) {
        auto branch_iter = std::find_if(this->m_branches.begin(), this->m_branches.end(), [&uptree_node, &downtree_node](const Branch& b) { return (b.uptree_node() == uptree_node && b.downtree_node() == downtree_node); });
        if (branch_iter != this->m_branches.end())
            this->m_branches.erase( branch_iter );

        this->m_nodes.erase( downtree_node );
    } else {
        for (const auto& branch : downtree_branches)
            this->drop_branch(branch.uptree_node(), branch.downtree_node());
    }
}


std::optional<Branch> ExtNetwork::uptree_branch(const std::string& node) const {
    if (!this->has_node(node))
        throw std::out_of_range("No such node: " + node);

    std::vector<Branch> branches;
    std::copy_if(this->m_branches.begin(), this->m_branches.end(), std::back_inserter(branches), [&node](const Branch& b) { return b.downtree_node() == node; });
    if (branches.empty())
        return {};

    if (branches.size() == 1)
        return std::move(branches[0]);

    throw std::logic_error("Bug - more than upstree branch for node: " + node);
}


std::vector<Branch> ExtNetwork::downtree_branches(const std::string& node) const {
    if (!this->has_node(node))
        throw std::out_of_range("No such node: " + node);

    std::vector<Branch> branches;
    std::copy_if(this->m_branches.begin(), this->m_branches.end(), std::back_inserter(branches), [&node](const Branch& b) { return b.uptree_node() == node; });
    return branches;
}

std::vector<const Branch*> ExtNetwork::branches() const {
    std::vector<const Branch*> branch_pointer;
    for (const auto& br : this->m_branches) {
        branch_pointer.emplace_back(&br);
    }
    return branch_pointer;
}

int ExtNetwork::NoOfBranches() const {
    return this->m_branches.size();
}

/*
  The validation of the network structure is very weak. The current validation
  goes as follows:

   1. A branch is defined with and uptree and downtree node; the node names used
      in the Branch definition is totally unchecked.

   2. When a node is added we check that the name of the node corresponds to a
      node name referred to in one of the previous branch definitions.

  The algorithm feels quite illogical, but from the documentation it seems to be
  the only possibility.
*/

void ExtNetwork::update_node(Node node)
{
    // This function should be called as a result of a NODEPROP deck
    // entry (or equivalent from restart file). So the node should
    // already exist, added in add_branch() from BRANPROP entries.
    std::string name = node.name();
    auto branch = std::find_if(this->m_branches.begin(), this->m_branches.end(),
                               [&name](const Branch& b) { return b.uptree_node() == name || b.downtree_node() == name;});

    if (branch == this->m_branches.end())
        throw std::invalid_argument("Node: " + name + " is not referenced by any branch and would be dangling.");


    if (branch->downtree_node() == name) {
        if (node.as_choke() && branch->vfp_table().has_value())
            throw std::invalid_argument("Node: " + name + " should serve as a choke => upstream branch can not have VFP table");
    }


    this->m_nodes.insert_or_assign(name, std::move(node) );
}

void ExtNetwork::add_indexed_node_name(std::string name)
{
    this->insert_indexed_node_names.emplace_back(name);
}

bool ExtNetwork::has_indexed_node_name(const std::string& name) const
{
    // Find given element in vector
    auto it = std::find(this->insert_indexed_node_names.begin(), this->insert_indexed_node_names.end(), name);

    return (it != this->insert_indexed_node_names.end());
}

std::vector<std::string> ExtNetwork::node_names() const
{
    return this->insert_indexed_node_names;
}
}
}
