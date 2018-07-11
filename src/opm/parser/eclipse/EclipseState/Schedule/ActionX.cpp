/*
  Copyright 2018 Statoil ASA.

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

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ActionX.hpp>


namespace Opm {

ActionX::ActionX(const std::string& name, size_t max_run, double max_wait) :
    m_name(name),
    max_run(max_run),
    max_wait(max_wait)
{}


ActionX::ActionX(const DeckKeyword& kw) {
    const auto& record = kw.getRecord(0);
    this->m_name = record.getItem("NAME").getTrimmedString(0);
    this->max_run = record.getItem("NUM").get<int>(0);
    this->max_wait = record.getItem("MAX_WAIT").getSIDouble(0);
}


void ActionX::addKeyword(const DeckKeyword& kw) {
    this->keywords.push_back(kw);
}

const std::string& ActionX::name() const {
    return this->m_name;
}

}
