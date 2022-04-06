/*
  Copyright 2018 NORCE.

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
#include <opm/input/eclipse/Schedule/Well/WVFPEXP.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include <string>
#include <vector>

namespace Opm {

    WVFPEXP WVFPEXP::serializeObject()
    {
        WVFPEXP result;
        result.m_extrapolate = true;
        result.m_shut = true;
        result.m_prevent = Prevent::No;

        return result;
    }

    bool WVFPEXP::operator==(const WVFPEXP& other) const {
        return (m_extrapolate == other.m_extrapolate)
            && (m_shut == other.m_shut)
            && (m_prevent == other.m_prevent);
    }

    void WVFPEXP::update(const DeckRecord& record) {
        const auto& exp_imp = record.getItem<ParserKeywords::WVFPEXP::EXPLICIT_IMPLICIT>().get<std::string>(0);
        const auto& close = record.getItem<ParserKeywords::WVFPEXP::CLOSE>().get<std::string>(0);
        const auto& prevent_thp = record.getItem<ParserKeywords::WVFPEXP::PREVENT_THP>().get<std::string>(0);
        //const auto& extrapolation_control = record.getItem<ParserKeywords::WVFPEXP::EXTRAPOLATION_CONTROL>().get<std::string>(0);
        m_extrapolate = (exp_imp == "EXP"); 
        m_shut = (close == "YES");
        if (prevent_thp == "YES1")
            m_prevent = Prevent::First;
        else if (prevent_thp == "YES2")
            m_prevent = Prevent::Every;
        else
            m_prevent = Prevent::No;
    }

    bool WVFPEXP::extrapolate() const {
        return m_extrapolate;
    }

    bool WVFPEXP::shut() const {
        return m_shut;
    }

    bool WVFPEXP::prevent() const {
        return m_prevent != Prevent::No;;
    }


    bool WVFPEXP::operator!=(const WVFPEXP& other) const {
        return !(*this == other);
    }

}
