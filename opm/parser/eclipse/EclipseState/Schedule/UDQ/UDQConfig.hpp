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


#ifndef UDQINPUT_HPP_
#define UDQINPUT_HPP_

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQDefine.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQAssign.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQParams.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>


namespace Opm {

    class DeckRecord;
    class Deck;
    class UDQConfig {
    public:
        explicit UDQConfig(const Deck& deck);
        const std::string& unit(const std::string& key) const;
        bool has_unit(const std::string& keyword) const;
        bool has_keyword(const std::string& keyword) const;
        void add_record(const DeckRecord& record);
        void assign_unit(const std::string& keyword, const std::string& unit);

        std::vector<UDQDefine> definitions() const;
        std::vector<UDQDefine> definitions(UDQVarType var_type) const;
        std::vector<UDQInput> input() const;

        // The size() method will return the number of active DEFINE and ASSIGN
        // statements; this will correspond to the length of the vector returned
        // from input().
        size_t size() const;

        std::vector<UDQAssign> assignments() const;
        std::vector<UDQAssign> assignments(UDQVarType var_type) const;
        const UDQParams& params() const;
        const UDQFunctionTable& function_table() const;
    private:
        UDQParams udq_params;
        UDQFunctionTable udqft;


        /*
          The choices of datastructures are strongly motivated by the
          constraints imposed by the Eclipse formatted restart files; for
          writing restart files it is essential to keep meticolous control over
          the ordering of the keywords. In this class the ordering is mainly
          maintained by the input_index map which keeps track of the insert
          order of each keyword, and whether the keyword is currently DEFINE'ed
          or ASSIGN'ed.
        */
        std::unordered_map<std::string, UDQDefine> m_definitions;
        std::unordered_map<std::string, UDQAssign> m_assignments;
        std::unordered_map<std::string, std::string> units;

        OrderedMap<std::string, UDQIndex> input_index;
        std::unordered_map<UDQVarType, std::size_t> type_count;
    };
}



#endif
