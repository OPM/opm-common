/*
  Copyright 2024 Norce

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
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/Tables/EzrokhiTable.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <cassert>

namespace Opm {

    void EzrokhiTable::init(const DeckRecord& record, const std::string& cname, const int icomp)
    {
        // DATA is a flattened (ncomps, 3) table
        const double c0 = record.getItem("DATA").getSIDouble(3 * icomp);
        const double c1 = record.getItem("DATA").getSIDouble(3 * icomp + 1);
        const double c2 = record.getItem("DATA").getSIDouble(3 * icomp + 2);

        // Store
        this->data.emplace(std::piecewise_construct, std::forward_as_tuple(cname), std::forward_as_tuple(c0, c1, c2));
    }

double EzrokhiTable::getC0(const std::string& name) const
{
    return this->data.at(name).c0;
}

double EzrokhiTable::getC1(const std::string& name) const
{
    return this->data.at(name).c1;
}

double EzrokhiTable::getC2(const std::string& name) const
{
    return this->data.at(name).c2;
}

EzrokhiTable::EzrokhiTable(const std::unordered_map<std::string, EzrokhiRecord>& records)
    : data{ records }
{}

std::size_t EzrokhiTable::size() const {
    return this->data.size();
}

bool EzrokhiTable::empty() const {
    return this->data.empty();
}

std::unordered_map<std::string, EzrokhiRecord>::const_iterator EzrokhiTable::begin() const {
    return this->data.begin();
}

std::unordered_map<std::string, EzrokhiRecord>::const_iterator EzrokhiTable::end() const {
    return this->data.end();
}

const EzrokhiRecord& EzrokhiTable::operator[](const std::string& name) const {
    return this->data.at(name);
}

EzrokhiTable EzrokhiTable::serializationTestObject() {
    return EzrokhiTable({{"comp1", {1.0, 2.0, 3.0}}, {"comp2", {2.0, 4.0, 6.0}}});
}

} // namespace Opm
