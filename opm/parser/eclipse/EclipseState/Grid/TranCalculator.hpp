/*
  Copyright 2020 Equinor AS.

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
#ifndef TRAN_CALCULATOR_HPP
#define TRAN_CALCULATOR_HPP

#include<string>

#include <opm/parser/eclipse/EclipseState/Grid/Keywords.hpp>


namespace Opm
{

namespace Fieldprops
{

enum class ScalarOperation {
    ADD = 1,
    EQUAL = 2,
    MUL = 3,
    MIN = 4,
    MAX = 5
};


class TranCalculator {
public:

    struct TranAction {
        ScalarOperation op;
        std::string field;
    };


    TranCalculator(const std::string& name_arg) :
        m_name(name_arg)
    {}

    std::string next_name() const {
        return this->m_name + std::to_string( this->actions.size() );
    }

    std::vector<TranAction>::const_iterator begin() const {
        return this->actions.begin();
    }

    std::vector<TranAction>::const_iterator end() const {
        return this->actions.end();
    }

    void add_action(ScalarOperation op, const std::string& field) {
        this->actions.push_back(TranAction{op, field});
    }

    std::size_t size() const {
        return this->actions.size();
    }

    const std::string& name() const {
        return this->m_name;
    }

    void clear() {
        this->actions.clear();
        this->actions.shrink_to_fit();
    }


    keywords::keyword_info<double> make_kw_info(ScalarOperation op) {
        keywords::keyword_info<double> kw_info;
        switch (op) {
        case ScalarOperation::MUL:
            kw_info.init(1);
            break;
        case ScalarOperation::ADD:
            kw_info.init(0);
            break;
        case ScalarOperation::MAX:
            kw_info.init(std::numeric_limits<double>::max());
            break;
        case ScalarOperation::MIN:
            kw_info.init(std::numeric_limits<double>::lowest());
            break;
        default:
            break;
        }
        return kw_info;
    }
private:
    std::string m_name;
    std::vector<TranAction> actions;
};

} // namespace Fieldprops
} // end namespace Opm
#endif // TRAN_CALCULATOR_HPP
