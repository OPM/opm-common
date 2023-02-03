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

#ifndef UDQASSIGN_HPP_
#define UDQASSIGN_HPP_

#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>

#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Opm {

class UDQAssign
{
public:
    // If the same keyword is assigned several times the different
    // assignment records are assembled in one UDQAssign instance.  This is
    // an attempt to support restart in a situation where a full UDQ ASSIGN
    // statement can be swapped with a UDQ DEFINE statement.
    struct AssignRecord
    {
        std::vector<std::string> input_selector{};
        std::unordered_set<std::string> rst_selector{};
        std::vector<UDQSet::EnumeratedWellItems> numbered_selector{};
        double value{};
        std::size_t report_step{};

        AssignRecord() = default;

        AssignRecord(const std::vector<std::string>& selector,
                     const double                    value_arg,
                     const std::size_t               report_step_arg)
            : input_selector(selector)
            , value         (value_arg)
            , report_step   (report_step_arg)
        {}

        AssignRecord(const std::unordered_set<std::string>& selector,
                     const double                           value_arg,
                     const std::size_t                      report_step_arg)
            : rst_selector(selector)
            , value       (value_arg)
            , report_step (report_step_arg)
        {}

        AssignRecord(const std::vector<UDQSet::EnumeratedWellItems>& selector,
                     const double                                    value_arg,
                     const std::size_t                               report_step_arg)
            : numbered_selector(selector)
            , value            (value_arg)
            , report_step      (report_step_arg)
        {}

        AssignRecord(std::vector<UDQSet::EnumeratedWellItems>&& selector,
                     const double                               value_arg,
                     const std::size_t                          report_step_arg)
            : numbered_selector(std::move(selector))
            , value            (value_arg)
            , report_step      (report_step_arg)
        {}

        void eval(UDQSet& values) const;

        bool operator==(const AssignRecord& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->input_selector);
            serializer(this->rst_selector);
            serializer(this->numbered_selector);
            serializer(this->value);
            serializer(this->report_step);
        }
    };

    UDQAssign() = default;
    UDQAssign(const std::string&              keyword,
              const std::vector<std::string>& selector,
              double                          value,
              std::size_t                     report_step);

    UDQAssign(const std::string&                     keyword,
              const std::unordered_set<std::string>& selector,
              double                                 value,
              std::size_t                            report_step);

    UDQAssign(const std::string&                              keyword,
              const std::vector<UDQSet::EnumeratedWellItems>& selector,
              double                                          value,
              std::size_t                                     report_step);

    UDQAssign(const std::string&                         keyword,
              std::vector<UDQSet::EnumeratedWellItems>&& selector,
              double                                     value,
              std::size_t                                report_step);

    static UDQAssign serializationTestObject();

    const std::string& keyword() const;
    UDQVarType var_type() const;

    void add_record(const std::vector<std::string>& selector,
                    double                          value,
                    std::size_t                     report_step);

    void add_record(const std::unordered_set<std::string>& rst_selector,
                    double                                 value,
                    std::size_t                            report_step);

    void add_record(const std::vector<UDQSet::EnumeratedWellItems>& selector,
                    double                                          value,
                    std::size_t                                     report_step);

    void add_record(std::vector<UDQSet::EnumeratedWellItems>&& selector,
                    double                                     value,
                    std::size_t                                report_step);

    UDQSet eval(const std::vector<UDQSet::EnumeratedWellItems>& items) const;
    UDQSet eval(const std::vector<std::string>& wells) const;
    UDQSet eval() const;
    std::size_t report_step() const;

    bool operator==(const UDQAssign& data) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_keyword);
        serializer(m_var_type);
        serializer(records);
    }

private:
    std::string m_keyword{};
    UDQVarType m_var_type{UDQVarType::NONE};
    std::vector<AssignRecord> records{};
};

} // namespace Opm

#endif // UDQASSIGN_HPP_
