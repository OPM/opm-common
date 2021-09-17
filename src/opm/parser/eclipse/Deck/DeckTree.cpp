/*
  Copyright 2021 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <opm/common/utility/FileSystem.hpp>
#include <opm/parser/eclipse/Deck/DeckTree.hpp>
namespace fs = Opm::filesystem;

namespace Opm {

DeckTree::TreeNode::TreeNode(const std::string& fn)
    : fname(fn)
    , size(0)
{}

DeckTree::TreeNode::TreeNode(const std::string& pn, const std::string& fn)
    : fname(fn)
    , size(0)
    , parent(pn)
{}

void DeckTree::TreeNode::add_include(const std::string& include_file) {
    this->include_files.emplace(include_file);
}

bool DeckTree::TreeNode::includes(const std::string& include_file) const {
    return this->include_files.count(include_file) > 0;
}


DeckTree::DeckTree(const std::string& fname)
    : root_file(fname)
{
    this->nodes.emplace( fname, TreeNode(fname) );
}

void DeckTree::add_root(const std::string& fname)
{
    if (this->root_file.has_value())
        throw std::logic_error("Root already assigned");

    auto abs_path = fs::absolute(fname);
    this->root_file = abs_path;
    this->nodes.emplace( abs_path, TreeNode(abs_path) );
}


bool DeckTree::includes_empty(const std::string& parent_file, const std::string& include_file) const {
    const auto& parent_node = this->nodes.at(parent_file);
    if (parent_node.size > 0)
        return false;

    if (parent_node.includes(include_file))
        return true;

    for (const auto& intermediate_file : parent_node.include_files) {
        if (this->includes_empty(intermediate_file, include_file))
            return true;
    }
    return false;
}


bool DeckTree::includes(const std::string& parent_file, const std::string& include_file) const {
    if (!this->root_file.has_value())
        return false;

    const auto& parent_node = this->nodes.at(parent_file);
    if (parent_node.includes(include_file))
        return true;

    for (const auto& intermediate_file : parent_node.include_files) {
        if (this->includes_empty(intermediate_file, include_file))
            return true;
    }

    return false;
}

const std::string& DeckTree::parent(const std::string& fname) const {
    std::string current_file = fname;
    while (true) {
        const auto& node = this->nodes.at(current_file);
        const auto& parent_node = this->nodes.at( node.parent.value() );
        if (parent_node.size > 0)
            return parent_node.fname;

        current_file = parent_node.fname;
    }
}

const std::string& DeckTree::root() const {
    return this->root_file.value();
}

void DeckTree::add_include(const std::string& parent_file, const std::string& include_file) {
    if (!this->root_file.has_value())
        return;

    auto& parent_node = this->nodes.at(parent_file);
    this->nodes.emplace(include_file, TreeNode(parent_file, include_file));
    parent_node.add_include( include_file );
}

void DeckTree::add_keyword(const std::string& fname) {
    if (!this->root_file.has_value())
        return;

    auto& node = this->nodes.at(fs::absolute(fname));
    node.size += 1;
}

std::size_t DeckTree::size(const std::string& fname) const {
    if (!this->root_file.has_value())
        return 0;

    const auto& node = this->nodes.at(fname);
    return node.size;
}


}
