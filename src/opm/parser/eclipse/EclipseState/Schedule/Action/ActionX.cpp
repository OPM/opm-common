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
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>


namespace Opm {




ActionX::ActionX(const std::string& name, size_t max_run, double min_wait, std::time_t start_time) :
    m_name(name),
    m_max_run(max_run),
    m_min_wait(min_wait),
    m_start_time(start_time)
{}


ActionX::ActionX(const DeckRecord& record, std::time_t start_time) :
    ActionX( record.getItem("NAME").getTrimmedString(0),
             record.getItem("NUM").get<int>(0),
             record.getItem("MIN_WAIT").getSIDouble(0),
             start_time )

{}


ActionX::ActionX(const DeckKeyword& kw, std::time_t start_time) :
    ActionX(kw.getRecord(0), start_time)
{
    std::vector<std::string> tokens;
    for (size_t record_index = 1; record_index < kw.size(); record_index++) {
        const auto& record = kw.getRecord(record_index);
        for (const auto& token : record.getItem("CONDITION").getData<std::string>())
            tokens.push_back(token);
    }
    this->ast = ActionAST(tokens);
}


void ActionX::addKeyword(const DeckKeyword& kw) {
    this->keywords.push_back(kw);
}



bool ActionX::eval(std::time_t sim_time, const ActionContext& /* context */) const {
    if (!this->ready(sim_time))
        return false;
    bool result = true;

    if (result) {
        this->run_count += 1;
        this->last_run = sim_time;
    }
    return result;
}


bool ActionX::ready(std::time_t sim_time) const {
  if (this->run_count >= this->max_run())
        return false;

    if (sim_time < this->start_time())
        return false;

    if (this->run_count == 0)
        return true;

    if (this->min_wait() <= 0)
        return true;

    return std::difftime(sim_time, this->last_run) > this->min_wait();
}


}
