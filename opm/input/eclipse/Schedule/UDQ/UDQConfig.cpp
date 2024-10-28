/*
  Copyright 2019 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>

#include <opm/io/eclipse/rst/state.hpp>
#include <opm/io/eclipse/rst/udq.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Schedule/MSW/SegmentMatcher.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQInput.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQSet.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQState.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/U.hpp> // UDQ

#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {
    std::string strip_quotes(const std::string& s)
    {
        if (s[0] == '\'') {
            return s.substr(1, s.size() - 2);
        }
        else {
            return s;
        }
    }

    class EvalAssign
    {
    public:
        static EvalAssign field()
        {
            return { []() {
                return [](const auto& assign) { return assign.eval(); };
            }};
        }

        static EvalAssign group(const std::size_t    report_step,
                                const Opm::Schedule& sched)
        {
            return { [report_step, &sched]() {
                return [groups = sched.groupNames(report_step)]
                    (const auto& assign)
                {
                    return assign.eval(groups);
                };
            }};
        }

        static EvalAssign well(const std::size_t    report_step,
                               const Opm::Schedule& sched)
        {
            return { [report_step, &sched]() {
                return [wells = sched.wellNames(report_step)]
                    (const auto& assign)
                {
                    return assign.eval(wells);
                };
            }};
        }

        static EvalAssign segment(const Opm::UDQContext& context)
        {
            return { [&context]() {
                // Result set should be sized according to total number of
                // segments.
                const auto segSet = context.segments();

                return [items = Opm::UDQSet::enumerateItems(segSet)](const auto& assign)
                {
                    return assign.eval(items);
                };
            }};
        }

        Opm::UDQSet operator()(const Opm::UDQAssign& assign) const
        {
            if (! this->eval_.has_value()) {
                // First call to operator().
                //
                // Create evaluation function using whatever state was
                // captured in the creation function, for instance a "const
                // UDQContext&".
                //
                // Note: This is deferred initialisation.  The create_()
                // call could be rather expensive so we don't incur the cost
                // of calling the function until we know we that we have to.
                this->eval_ = this->create_();
            }

            // Evaluate type dependent UDQ ASSIGN statement.
            return (*this->eval_)(assign);
        }

    private:
        using Eval = std::function<Opm::UDQSet(const Opm::UDQAssign&)>;
        using Create = std::function<Eval()>;

        Create create_;
        mutable std::optional<Eval> eval_;

        EvalAssign(Create create) : create_{ std::move(create) } {}
    };
} // Anonymous namespace

namespace Opm {

    UDQConfig::UDQConfig(const UDQParams& params)
        : udq_params { params }
        , udqft      { udq_params }
    {}

    UDQConfig::UDQConfig(const UDQParams&           params,
                         const RestartIO::RstState& rst_state)
        : UDQConfig { params }
    {
        for (const auto& udq : rst_state.udqs) {
            if (udq.isDefine()) {
                this->add_define(udq, rst_state.header.report_step);
            }
            else {
                this->add_assign(udq, rst_state.header.report_step);
            }

            this->add_unit(udq.name, udq.unit);
        }
    }

    UDQConfig UDQConfig::serializationTestObject()
    {
        UDQConfig result;
        result.udq_params = UDQParams::serializationTestObject();
        result.udqft = UDQFunctionTable(result.udq_params);
        result.m_definitions = {{"test1", UDQDefine::serializationTestObject()}};
        result.m_assignments = {{"test2", UDQAssign::serializationTestObject()}};
        result.m_tables = {{"test3", UDT::serializationTestObject()}};
        result.units = {{"test3", "test4"}};
        result.input_index.insert({"test5", UDQIndex::serializationTestObject()});
        result.type_count = {{UDQVarType::SCALAR, 5}};
        result.pending_assignments_.push_back("test2");

        return result;
    }

    const std::string& UDQConfig::unit(const std::string& key) const
    {
        const auto pair_ptr = this->units.find(key);
        if (pair_ptr == this->units.end()) {
            throw std::invalid_argument("No such UDQ quantity: " + key);
        }

        return pair_ptr->second;
    }

    bool UDQConfig::has_unit(const std::string& keyword) const
    {
        return this->units.find(keyword) != this->units.end();
    }

    bool UDQConfig::has_keyword(const std::string& keyword) const
    {
        return (this->m_assignments.find(keyword) != this->m_assignments.end())
            || (this->m_definitions.find(keyword) != this->m_definitions.end());
    }

    void UDQConfig::add_record(SegmentMatcherFactory  create_segment_matcher,
                               const DeckRecord&      record,
                               const KeywordLocation& location,
                               const std::size_t      report_step)
    {
        using KW = ParserKeywords::UDQ;

        const auto action = UDQ::actionType(record.getItem<KW::ACTION>().get<RawString>(0));
        const auto& quantity = record.getItem<KW::QUANTITY>().get<std::string>(0);
        const auto data = RawString::strings(record.getItem<KW::DATA>().getData<RawString>());

        if (action == UDQAction::UPDATE) {
            this->add_update(quantity, report_step, location, data);
        }
        else if (action == UDQAction::UNITS) {
            this->add_unit(quantity, data.front());
        }
        else if (action == UDQAction::ASSIGN) {
            auto selector = std::vector<std::string>(data.begin(), data.end() - 1);
            std::transform(selector.cbegin(), selector.cend(), selector.begin(), strip_quotes);
            const auto value = std::stod(data.back());
            this->add_assign(quantity,
                             std::move(create_segment_matcher),
                             selector, value, report_step);
        }
        else if (action == UDQAction::DEFINE) {
            this->add_define(quantity, location, data, report_step);
        }
        else {
            throw std::runtime_error {
                "Unknown UDQ Operation " + std::to_string(static_cast<int>(action))
            };
        }
    }

    void UDQConfig::add_unit(const std::string& keyword, const std::string& quoted_unit)
    {
        const auto Unit = strip_quotes(quoted_unit);
        const auto pair_ptr = this->units.find(keyword);
        if (pair_ptr != this->units.end()) {
            if (pair_ptr->second != Unit) {
                throw std::invalid_argument("Illegal to change unit of UDQ keyword runtime");
            }

            return;
        }

        this->units[keyword] = Unit;
    }

    void UDQConfig::add_update(const std::string&              keyword,
                               const std::size_t               report_step,
                               const KeywordLocation&          location,
                               const std::vector<std::string>& data)
    {
        if (data.empty()) {
            throw OpmInputError {
                fmt::format("Missing third item: ON|OFF|NEXT for UDQ update of {}", keyword),
                location
            };
        }

        if (this->m_definitions.count(keyword) == 0) {
            if (this->m_assignments.count(keyword) > 0) {
                OpmLog::warning(fmt::format("UDQ variable {} is constant, so UPDATE will have no effect.", keyword));
                return;
            }

            throw OpmInputError {
                fmt::format("UDQ variable: {} must be defined before you can use UPDATE", keyword),
                location
            };
        }

        const auto update_status = UDQ::updateType(data[0]);

        auto& def = this->m_definitions[keyword];
        def.update_status(update_status, report_step);
    }

    void UDQConfig::add_assign(const std::string&              quantity,
                               SegmentMatcherFactory           create_segment_matcher,
                               const std::vector<std::string>& selector,
                               const double                    value,
                               const std::size_t               report_step)
    {
        this->add_node(quantity, UDQAction::ASSIGN);

        switch (UDQ::varType(quantity)) {
        case UDQVarType::SEGMENT_VAR:
            this->add_enumerated_assign(quantity,
                                        std::move(create_segment_matcher),
                                        selector, value, report_step);
            break;

        default:
            this->add_assign_impl(quantity, selector, value, report_step);
            break;
        }

        if (this->m_assignments.find(quantity) != this->m_assignments.end()) {
            this->pending_assignments_.push_back(quantity);
        }
    }

    void UDQConfig::add_define(const std::string&              quantity,
                               const KeywordLocation&          location,
                               const std::vector<std::string>& expression,
                               const std::size_t               report_step)
    {
        this->add_node(quantity, UDQAction::DEFINE);

        this->m_definitions.insert_or_assign(quantity,
                                             UDQDefine {
                                                 this->udq_params,
                                                 quantity,
                                                 report_step,
                                                 location,
                                                 expression
                                             });

        this->define_order.insert(quantity);
    }

    void UDQConfig::add_table(const std::string& name, UDT udt)
    {
        m_tables.emplace(name, std::move(udt));
    }

    bool UDQConfig::clear_pending_assignments()
    {
        const auto update = ! this->pending_assignments_.empty();
        this->pending_assignments_.clear();

        return update;
    }

    void UDQConfig::eval_assign(const std::size_t     report_step,
                                const Schedule&       sched,
                                const WellMatcher&    wm,
                                SegmentMatcherFactory create_segment_matcher,
                                SummaryState&         st,
                                UDQState&             udq_state) const
    {
        auto factories = UDQContext::MatcherFactories{};
        factories.segments = std::move(create_segment_matcher);

        auto context = UDQContext {
            this->function_table(), wm, this->m_tables,
            std::move(factories), st, udq_state
        };

        this->eval_assign(report_step, sched, context);
    }

    void UDQConfig::eval(const std::size_t       report_step,
                         const Schedule&         sched,
                         const WellMatcher&      wm,
                         SegmentMatcherFactory   create_segment_matcher,
                         RegionSetMatcherFactory create_region_matcher,
                         SummaryState&           st,
                         UDQState&               udq_state) const
    {
        auto factories = UDQContext::MatcherFactories {};
        factories.segments = std::move(create_segment_matcher);
        factories.regions  = std::move(create_region_matcher);

        auto context = UDQContext {
            this->function_table(), wm, this->m_tables,
            std::move(factories), st, udq_state
        };

        this->eval_assign(report_step, sched, context);
        this->eval_define(report_step, udq_state, context);
    }

    const UDQDefine& UDQConfig::define(const std::string& key) const
    {
        return this->m_definitions.at(key);
    }

    const UDQAssign& UDQConfig::assign(const std::string& key) const
    {
        return this->m_assignments.at(key);
    }

    std::vector<UDQDefine> UDQConfig::definitions() const
    {
        std::vector<UDQDefine> ret;

        for (const auto& [key, index] : this->input_index) {
            if (index.action == UDQAction::DEFINE) {
                ret.push_back(this->m_definitions.at(key));
            }
        }

        return ret;
    }

    std::vector<UDQDefine> UDQConfig::definitions(const UDQVarType var_type) const
    {
        std::vector<UDQDefine> filtered_defines;

        for (const auto& [key, index] : this->input_index) {
            if (index.action == UDQAction::DEFINE) {
                const auto& udq_define = this->m_definitions.at(key);
                if (udq_define.var_type() == var_type) {
                    filtered_defines.push_back(udq_define);
                }
            }
        }

        return filtered_defines;
    }

    std::vector<UDQInput> UDQConfig::input() const
    {
        std::vector<UDQInput> res;

        for (const auto& [key, index] : this->input_index) {
            const auto u = this->has_unit(key)
                ? this->unit(key) : std::string{};

            if (index.action == UDQAction::DEFINE) {
                res.emplace_back(index, &this->m_definitions.at(key), u);
            }
            else if (index.action == UDQAction::ASSIGN) {
                res.emplace_back(index, &this->m_assignments.at(key), u);
            }
        }

        return res;
    }

    void UDQConfig::exportTypeCount(std::array<int, static_cast<std::size_t>(UDQVarType::NumTypes)>& count) const
    {
        count.fill(0);

        for (const auto& [type, cnt] : this->type_count) {
            count[static_cast<std::size_t>(type)] = cnt;
        }
    }

    std::size_t UDQConfig::size() const
    {
        return std::count_if(this->input_index.begin(), this->input_index.end(),
                             [](const auto& index_pair)
                             {
                                 const auto action = index_pair.second.action;

                                 return (action == UDQAction::DEFINE)
                                     || (action == UDQAction::ASSIGN);
                             });
    }

    UDQInput UDQConfig::operator[](const std::string& keyword) const
    {
        auto index_iter = this->input_index.find(keyword);
        if (index_iter == this->input_index.end()) {
            throw std::invalid_argument("Keyword: '" + keyword +
                                        "' not recognized as ASSIGN/DEFINE UDQ");
        }

        const auto u = this->has_unit(keyword)
            ? this->unit(keyword) : std::string{};

        if (index_iter->second.action == UDQAction::ASSIGN) {
            return { index_iter->second, &this->m_assignments.at(keyword), u };
        }

        if (index_iter->second.action == UDQAction::DEFINE) {
            return { index_iter->second, &this->m_definitions.at(keyword), u };
        }

        throw std::logic_error("Internal error - should not be here");
    }

    UDQInput UDQConfig::operator[](const std::size_t insert_index) const
    {
        auto index_iter = std::find_if(this->input_index.begin(), this->input_index.end(),
            [insert_index](const auto& name_index)
        {
            return name_index.second.insert_index == insert_index;
        });

        if (index_iter == this->input_index.end()) {
            throw std::invalid_argument("Insert index not recognized");
        }

        const auto& [keyword, index] = *index_iter;
        const auto u = this->has_unit(keyword)
            ? this->unit(keyword) : std::string{};

        if (index.action == UDQAction::ASSIGN) {
            return { index, &this->m_assignments.at(keyword), u };
        }

        if (index.action == UDQAction::DEFINE) {
            return { index, &this->m_definitions.at(keyword), u };
        }

        throw std::logic_error("Internal error - should not be here");
    }

    std::vector<UDQAssign> UDQConfig::assignments() const
    {
        std::vector<UDQAssign> ret;

        for (const auto& [key, Input] : this->input_index) {
            if (Input.action == UDQAction::ASSIGN) {
                ret.push_back(this->m_assignments.at(key));
            }
        }

        return ret;
    }

    std::vector<UDQAssign> UDQConfig::assignments(const UDQVarType var_type) const
    {
        std::vector<UDQAssign> filtered_assigns;

        for (const auto& index_pair : this->input_index) {
            auto assign_iter = this->m_assignments.find(index_pair.first);
            if ((assign_iter != this->m_assignments.end()) &&
                (assign_iter->second.var_type() == var_type))
            {
                filtered_assigns.push_back(assign_iter->second);
            }
        }

        return filtered_assigns;
    }

    const UDQParams& UDQConfig::params() const
    {
        return this->udq_params;
    }

    const UDQFunctionTable& UDQConfig::function_table() const
    {
        return this->udqft;
    }

    const std::unordered_map<std::string, UDT>& UDQConfig::tables() const
    {
        return m_tables;
    }

    bool UDQConfig::operator==(const UDQConfig& data) const
    {
        return (this->params() == data.params())
            && (this->function_table() == data.function_table())
            && (this->m_definitions == data.m_definitions)
            && (this->m_assignments == data.m_assignments)
            && (this->m_tables == data.m_tables)
            && (this->units == data.units)
            && (this->define_order == data.define_order)
            && (this->input_index == data.input_index)
            && (this->type_count == data.type_count)
            && (this->pending_assignments_ == data.pending_assignments_)
            ;
    }

    void UDQConfig::required_summary(std::unordered_set<std::string>& summary_keys) const
    {
        for (const auto& def_pair : this->m_definitions) {
            def_pair.second.required_summary(summary_keys);
        }
    }

    // ===========================================================================
    // Private member functions below separator
    // ===========================================================================

    void UDQConfig::add_node(const std::string& quantity, const UDQAction action)
    {
        auto index_iter = this->input_index.find(quantity);
        if (this->input_index.find(quantity) == this->input_index.end()) {
            auto var_type = UDQ::varType(quantity);
            auto insert_index = this->input_index.size();

            this->type_count[var_type] += 1;
            this->input_index[quantity] = UDQIndex(insert_index, this->type_count[var_type], action, var_type);
        }
        else {
            index_iter->second.action = action;
        }
    }

    void UDQConfig::add_assign(const RestartIO::RstUDQ& udq,
                               const std::size_t        report_step)
    {
        this->add_node(udq.name, UDQAction::ASSIGN);

        this->add_assign_impl(udq.name, udq, report_step);
    }

    void UDQConfig::add_define(const RestartIO::RstUDQ& udq,
                               const std::size_t        report_step)
    {
        this->add_define(udq.name, KeywordLocation { "UDQ", "Restart file", 0 },
                         std::vector { std::string { udq.definingExpression() } },
                         report_step);

        auto pos = this->m_definitions.find(udq.name);
        assert (pos != this->m_definitions.end());

        pos->second.update_status(udq.currentUpdateStatus(), report_step);
    }

    void UDQConfig::eval_assign(const std::size_t report_step,
                                const Schedule&   sched,
                                UDQContext&       context) const
    {
        if (this->pending_assignments_.empty()) {
            return;             // Nothing to do
        }

        const auto handlers = std::map<UDQVarType, EvalAssign> {
            { UDQVarType::FIELD_VAR  , EvalAssign::field()                   },
            { UDQVarType::GROUP_VAR  , EvalAssign::group(report_step, sched) },
            { UDQVarType::WELL_VAR   , EvalAssign::well(report_step, sched)  },
            { UDQVarType::SEGMENT_VAR, EvalAssign::segment(context)          },
        };

        // Recall: pending_assignments_ is mutable.
        auto pending = std::vector<std::string>{};
        this->pending_assignments_.swap(pending);

        {
            std::sort(pending.begin(), pending.end());
            auto u = std::unique(pending.begin(), pending.end());
            pending.erase(u, pending.end());
        }

        for (const auto& assignment : pending) {
            auto asgn_pos = this->m_assignments.find(assignment);
            if (asgn_pos == this->m_assignments.end()) {
                // No such ASSIGNment.  Unexpected.
                continue;
            }

            auto handler = handlers.find(asgn_pos->second.var_type());
            if (handler == handlers.end()) {
                // Unhandled/unsupported variable type.
                continue;
            }

            context.update_assign(assignment, handler->second(asgn_pos->second));
        }
    }

    void UDQConfig::eval_define(const std::size_t report_step,
                                const UDQState&   udq_state,
                                UDQContext&       context) const
    {
        auto var_type_bit = [](const UDQVarType var_type)
        {
            return 1ul << static_cast<std::size_t>(var_type);
        };

        auto select_var_type = std::size_t{0};
        select_var_type |= var_type_bit(UDQVarType::WELL_VAR);
        select_var_type |= var_type_bit(UDQVarType::GROUP_VAR);
        select_var_type |= var_type_bit(UDQVarType::FIELD_VAR);
        select_var_type |= var_type_bit(UDQVarType::SEGMENT_VAR);

        for (const auto& [keyword, index] : this->input_index) {
            if (index.action != UDQAction::DEFINE) {
                continue;
            }

            auto def_pos = this->m_definitions.find(keyword);
            if (def_pos == this->m_definitions.end()) { // No such def
                throw std::logic_error {
                    fmt::format("Internal error: UDQ '{}' is not among "
                                "those DEFINEd for numerical evaluation", keyword)
                };
            }

            const auto& def = def_pos->second;
            if (((select_var_type & var_type_bit(def.var_type())) == 0) || // Unwanted Var Type
                ! udq_state.define(keyword, def.status())) // UDQ def not applicable now
            {
                continue;
            }

            context.update_define(report_step, keyword, def.eval(context));
        }
    }

    void UDQConfig::add_enumerated_assign(const std::string&              quantity,
                                          SegmentMatcherFactory           create_segment_matcher,
                                          const std::vector<std::string>& selector,
                                          const double                    value,
                                          const std::size_t               report_step)
    {
        auto segmentMatcher = create_segment_matcher();

        auto setDescriptor = SegmentMatcher::SetDescriptor{};
        if (! selector.empty()) {
            setDescriptor.wellNames(selector.front());
        }
        if (selector.size() > 1) {
            setDescriptor.segmentNumber(selector[1]);
        }

        auto items = UDQSet::
            enumerateItems(segmentMatcher->findSegments(setDescriptor));

        this->add_assign_impl(quantity, std::move(items), value, report_step);
    }

} // namespace Opm
