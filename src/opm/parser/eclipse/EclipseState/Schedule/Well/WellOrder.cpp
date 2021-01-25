/*
  Copyright 2021 Equinor ASA.

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
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellOrder.hpp>

namespace Opm {

void WellOrder::add(const std::string& well) {
    auto iter = this->m_wells1.find( well );
    if (iter == this->m_wells1.end()) {
        std::size_t insert_index = this->m_wells2.size();
        this->m_wells1.emplace( well, insert_index );
        this->m_wells2.push_back( well );
    }
}

WellOrder::WellOrder(const std::vector<std::string>& wells) {
    for (const auto& w : wells)
        this->add(w);
}

bool WellOrder::has(const std::string& wname) const {
    return (this->m_wells1.count(wname) != 0);
}


const std::vector<std::string>& WellOrder::wells() const {
    return this->m_wells2;
}

std::vector<std::string> WellOrder::sort(std::vector<std::string> wells) const {
    std::sort(wells.begin(), wells.end(), [this](const std::string& w1, const std::string& w2) -> bool { return this->m_wells1.at(w1) < this->m_wells1.at(w2); });
    return wells;
}

WellOrder WellOrder::serializeObject() {
    WellOrder wo;
    wo.add("W1");
    wo.add("W2");
    wo.add("W3");
    return wo;
}

std::vector<std::string>::const_iterator WellOrder::begin() const {
    return this->m_wells2.begin();
}

std::vector<std::string>::const_iterator WellOrder::end() const {
    return this->m_wells2.end();
}

bool WellOrder::operator==(const WellOrder& other) const {
    return this->m_wells1 == other.m_wells1 &&
           this->m_wells2 == other.m_wells2;
}

}
