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

#ifndef EXT_NETWORK_HPP
#define EXT_NETWORK_HPP

#include <opm/input/eclipse/Schedule/Network/Branch.hpp>
#include <opm/input/eclipse/Schedule/Network/Node.hpp>

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace Opm {
    class Schedule;
} // namespace Opm

namespace Opm::Network {

class ExtNetwork
{
public:
    ExtNetwork() = default;
    bool active() const;
    bool is_standard_network() const;
    void set_standard_network(bool is_standard_network);
    void add_branch(Branch branch);
    void add_or_replace_branch(Branch branch);
    void drop_branch(const std::string& uptree_node, const std::string& downtree_node);
    bool has_node(const std::string& name) const;
    void update_node(Node node);
    const Node& node(const std::string& name) const;
    std::vector<std::reference_wrapper<const Node>> roots() const;
    std::vector<Branch> downtree_branches(const std::string& node) const;
    std::vector<const Branch*> branches() const;
    std::optional<Branch> uptree_branch(const std::string& node) const;
    const std::vector<std::string>& node_names() const;
    std::set<std::string> leaf_nodes() const;
    int NoOfBranches() const;
    int NoOfNodes() const;

    bool operator==(const ExtNetwork& other) const;
    static ExtNetwork serializationTestObject();

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_branches);
        serializer(insert_indexed_node_names);
        serializer(m_nodes);
        serializer(m_is_standard_network);
    }

private:
    std::vector<Branch> m_branches;
    std::vector<std::string> insert_indexed_node_names;
    std::map<std::string, Node> m_nodes;
    bool m_is_standard_network{false};

    bool has_indexed_node_name(const std::string& name) const;
    void add_indexed_node_name(const std::string& name);
};

} // namespace Opm::Network

#endif // EXT_NETWORK_HPP
