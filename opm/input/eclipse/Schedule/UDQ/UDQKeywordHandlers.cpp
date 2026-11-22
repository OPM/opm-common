/*
  Copyright 2020 Statoil ASA.

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

#include "UDQKeywordHandlers.hpp"

#include "../HandlerContext.hpp"

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/U.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>
#include <opm/input/eclipse/Schedule/MSW/SegmentMatcher.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>

#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <memory>
#include <optional>

namespace {

    std::optional<Opm::UDQConfig::DynamicSelector>
    makeDynamicSelector(const Opm::HandlerContext& handlerContext)
    {
        auto dynSelector = std::optional<Opm::UDQConfig::DynamicSelector>{};

        if (handlerContext.action_mode) {
            dynSelector = Opm::UDQConfig::DynamicSelector{};

            if (const auto dynWells = handlerContext.matches.wells();
                ! dynWells.empty())
            {
                dynSelector->wells(dynWells.begin(), dynWells.end());
            }
        }

        return dynSelector;
    }

    class EvaluationCheck
    {
    public:
        explicit EvaluationCheck(std::vector<std::string>   defines,
                                 const Opm::HandlerContext& ctx,
                                 const Opm::UDQConfig&      udqs);

        bool feasible() const { return this->message_.empty(); }

        const std::string& message() const { return this->message_; }

    private:
        std::string message_{};
    };

    EvaluationCheck::EvaluationCheck(std::vector<std::string>   defines,
                                     const Opm::HandlerContext& ctx,
                                     const Opm::UDQConfig&      udqs)
    {
        for (const auto& define : defines) {
            const auto reqObj = udqs.define(define).requiredObjects();

            auto missingWells = std::vector<std::string>{};
            std::ranges::copy_if(reqObj.wells, std::back_inserter(missingWells),
                                 [&ctx](const auto& reqWellPatt)
                                 { return !ctx.hasWell(reqWellPatt); });

            auto missingGroups = std::vector<std::string>{};
            std::ranges::copy_if(reqObj.groups, std::back_inserter(missingGroups),
                                 [&ctx](const auto &reqGrpPatt)
                                 { return !ctx.hasGroup(reqGrpPatt); });

            if (missingWells.empty() && missingGroups.empty()) {
                // No missing wells and no missing groups.  This is fine.
                continue;
            }

            // If we get here then there are missing objects (wells and/or
            // groups).  Report those.
            this->message_ += fmt::format("  Missing object{} in "
                                          "defining expression for {}\n",
                                          (missingGroups.size() + missingWells.size() != 1)
                                          ? "s" : "", define);

            if (! missingWells.empty()) {
                this->message_ += "    -> No existing well matches ";

                if (missingWells.size() == 1) {
                    this->message_ += fmt::format("the name {}\n", missingWells.front());
                }
                else {
                    this->message_ += fmt::format("any of the names {:n}\n", missingWells);
                }
            }

            if (! missingGroups.empty()) {
                this->message_ += "    -> No existing group matches ";

                if (missingGroups.size() == 1) {
                    this->message_ += fmt::format("the name {}\n", missingGroups.front());
                }
                else {
                    this->message_ += fmt::format("any of the names {:n}\n", missingGroups);
                }
            }
        }
    }

} // Anonymous namespace

namespace Opm {

namespace {

void handleUDQ(HandlerContext& handlerContext)
{
    auto new_udq = handlerContext.state().udq();

    auto segment_matcher_factory = [&handlerContext]()
    {
        return std::make_unique<SegmentMatcher>(handlerContext.state());
    };

    const auto dynSelector = makeDynamicSelector(handlerContext);

    auto newDefines = std::vector<std::string>{};

    for (const auto& record : handlerContext.keyword) {
        const auto definition = new_udq
            .add_record(segment_matcher_factory, record,
                        handlerContext.keyword.location(),
                        handlerContext.currentStep,
                        dynSelector);

        if (definition.has_value()) {
            newDefines.push_back(*definition);
        }
    }

    if (const auto eval = EvaluationCheck {
            std::move(newDefines), handlerContext, new_udq
        }; ! eval.feasible())
    {
        const auto msg_fmt = fmt::format(R"(Problem with {{keyword}}
In {{file}} line {{line}}
DEFINE cannot be evaluated:
{})", eval.message());

        handlerContext.parseContext
            .handleError(ParseContext::UDQ_DEFINE_CANNOT_EVAL,
                         msg_fmt,
                         handlerContext.keyword.location(),
                         handlerContext.errors);
    }

    handlerContext.state().udq.update(std::move(new_udq));
}

void handleUDT(HandlerContext& handlerContext)
{
    auto new_udq = handlerContext.state().udq();

    using PUDT = ParserKeywords::UDT;

    const auto& header = handlerContext.keyword.getRecord(0);
    const std::string name = header.getItem<PUDT::TABLE_NAME>().get<std::string>(0);

    const int dim = header.getItem<PUDT::DIMENSIONS>().get<int>(0);
    if (dim != 1) {
        throw OpmInputError("Only 1D UDTs are supported",
                            handlerContext.keyword.location());

    }

    const auto& points = handlerContext.keyword.getRecord(1);
    const std::string interp_type = points.getItem<PUDT::INTERPOLATION_TYPE>().get<std::string>(0);
    UDT::InterpolationType type;
    if (interp_type == "NV") {
        type = UDT::InterpolationType::NearestNeighbour;
    } else if (interp_type == "LC") {
        type = UDT::InterpolationType::LinearClamp;
    } else if (interp_type == "LL") {
        type = UDT::InterpolationType::LinearExtrapolate;
    } else {
        throw OpmInputError(fmt::format("Unknown UDT interpolation type {}", interp_type),
                            handlerContext.keyword.location());
    }
    const auto x_vals = points.getItem<ParserKeywords::UDT::INTERPOLATION_POINTS>().getData<double>();

    if (!std::ranges::is_sorted(x_vals)) {
        throw OpmInputError("UDT: Interpolation points need to be given in ascending order",
                            handlerContext.keyword.location());
    }

    if (auto it = std::ranges::adjacent_find(x_vals); it != x_vals.end()) {
        throw OpmInputError(fmt::format("UDT: Interpolation points need to be unique: "
                                        "found duplicate for {}", *it),
                            handlerContext.keyword.location());
    }

    const auto& data = handlerContext.keyword.getRecord(2);
    const auto y_vals = data.getItem<ParserKeywords::UDT::TABLE_VALUES>().getData<double>();

    if (x_vals.size() != y_vals.size()) {
        throw OpmInputError(fmt::format("UDT data size mismatch, number of x-values {}",
                                        ", number of y-values {}",
                                        x_vals.size(), y_vals.size()),
                            handlerContext.keyword.location());
    }

    new_udq.add_table(name, UDT(x_vals, y_vals, type));

    handlerContext.state().udq.update(std::move(new_udq));
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getUDQHandlers()
{
    return {
        { "UDQ", &handleUDQ },
        { "UDT", &handleUDT },
    };
}

}
