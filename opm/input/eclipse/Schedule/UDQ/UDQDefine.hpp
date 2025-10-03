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

#ifndef UDQ_DEFINE_HPP
#define UDQ_DEFINE_HPP

#include <opm/input/eclipse/Schedule/UDQ/UDQContext.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQFunctionTable.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQToken.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Opm {

class UDQASTNode;
class ParseContext;
class ErrorGuard;

} // Namespace Opm

namespace Opm {

class UDQDefine
{
public:
    UDQDefine();

    UDQDefine(const UDQParams& udq_params,
              const std::string& keyword,
              std::size_t report_step,
              const KeywordLocation& location,
              const std::vector<std::string>& deck_data);

    UDQDefine(const UDQParams& udq_params,
              const std::string& keyword,
              std::size_t report_step,
              const KeywordLocation& location,
              const std::vector<std::string>& deck_data,
              const ParseContext& parseContext,
              ErrorGuard& errors);

    template <typename T>
    UDQDefine(const UDQParams& udq_params,
              const std::string& keyword,
              std::size_t report_step,
              const KeywordLocation& location,
              const std::vector<std::string>& deck_data,
              const ParseContext& parseContext,
              T&& errors);

    static UDQDefine serializationTestObject();

    /// All specific objects required for the defining expression.
    ///
    /// Could, for instance, be a collection of specific well names in a
    /// field level UDQ, or a set of group name patterns for a group level
    /// UDQ.
    UDQ::RequisiteEvaluationObjects requiredObjects() const;

    UDQSet eval(const UDQContext& context) const;
    const std::string& keyword() const;
    const std::string& input_string() const { return this->input_string_; }
    const KeywordLocation& location() const;
    UDQVarType var_type() const;
    std::set<UDQTokenType> func_tokens() const;
    void required_summary(std::unordered_set<std::string>& summary_keys) const;
    void update_status(UDQUpdate update_status, std::size_t report_step);
    std::pair<UDQUpdate, std::size_t> status() const;
    const std::vector<Opm::UDQToken>& tokens() const;
    void clear_next() const
    {
        if (this->m_update_status == UDQUpdate::NEXT) {
            this->m_update_status = UDQUpdate::OFF;
        }
    }

    /// Clear "UPDATE NEXT" flag
    ///
    /// This is required by the way we form ScheduleState objects.  The
    /// function resets UPDATE NEXT to UPDATE OFF, and should typically be
    /// called at the end of a report step/beginning of the next report
    /// step.  If we do not do this, then a UDQ define statement with an
    /// UPDATE NEXT status will behave as if there is an implicit UPDATE
    /// NEXT statement at the beginning of each subsequent report step and
    /// that, in turn, will generate unwanted value updates for the
    /// quantity.
    ///
    /// \return Whether or not UPDATE NEXT was reset to UPDATE OFF.  Allows
    /// client code to take action, if needed, based on the knowledge that
    /// all such value updates have been applied and to prepare for the next
    /// report step.
    bool clear_update_next_for_new_report_step()
    {
        if (this->m_update_status == UDQUpdate::NEXT) {
            this->m_update_status = UDQUpdate::OFF;

            return true;
        }

        return false;
    }

    bool operator==(const UDQDefine& data) const;

    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_keyword);
        serializer(input_string_);
        serializer(m_tokens);
        serializer(ast);
        serializer(m_var_type);
        serializer(m_location);
        serializer(m_update_status);
        serializer(m_report_step);
    }

private:
    std::string m_keyword{};
    std::string input_string_{};
    std::vector<Opm::UDQToken> m_tokens{};
    std::shared_ptr<UDQASTNode> ast{};
    UDQVarType m_var_type{UDQVarType::NONE};
    KeywordLocation m_location{};
    std::size_t m_report_step{};
    mutable UDQUpdate m_update_status{UDQUpdate::NEXT};

    UDQSet scatter_scalar_value(UDQSet&& res, const UDQContext& context) const;
    UDQSet scatter_scalar_well_value(const UDQContext& context, const std::optional<double>& value) const;
    UDQSet scatter_scalar_group_value(const UDQContext& context, const std::optional<double>& value) const;
    UDQSet scatter_scalar_segment_value(const UDQContext& context, const std::optional<double>& value) const;
};

} // Namespace Opm

#endif // UDQ_DEFINE_HPP
