/*
  Copyright 2021 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <opm/io/eclipse/rst/action.hpp>

#include <opm/output/eclipse/VectorItems/action.hpp>

#include <opm/common/utility/String.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <fmt/format.h>

#include <array>
#include <cmath>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

    /// Export quantity name and arguments of the given restart
    /// file action condition quantity into the given token vector.
    ///
    /// \param[in] quantity The quantity to capture.
    ///
    /// \param[in,out] tokens The vector into which to export the quantity's tokens.
    void captureQuantityTokens(const Opm::RestartIO::RstAction::Quantity& quantity,
                               std::vector<std::string>&                  tokens)
    {
        tokens.push_back(quantity.quantity());

        if (const auto& args = quantity.args(); !args.empty()) {
            tokens.insert(tokens.end(), args.begin(), args.end());
        }
    }

    /// Whether or not a given double value is a numeric integer.
    ///
    /// A value is deemed to be an integer if it is finite and
    /// has no fractional part.
    ///
    /// \param[in] value The double value to check.
    ///
    /// \return True if the value is a numeric integer, false otherwise.
    bool isNumericInteger(const double value)
    {
        return std::isfinite(value) && (std::fmod(value, 1.0) == 0.0);
    }

    // --------------------------------------------------------------------------

    /// Construct argument vector for a region level quantity from restart file information.
    ///
    /// \param[in] regionIDIndex The index of the region ID in the IACN vector.
    /// Expected to be either LHSRegionID or RHSRegionID.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the region level
    /// quantity.  Empty if the region ID is zero or negative, otherwise contains
    /// the string representation of a single region ID.
    std::vector<std::string>
    regionArgs(const Opm::RestartIO::Helpers::VectorItems::IACN::index regionIDIndex,
               std::span<const int>                                    iacn)
    {
        if (const auto regID = iacn[regionIDIndex]; regID > 0) {
            return { fmt::format("{}", regID) };
        }

        return {};
    }

    /// Construct argument vector for a segment level quantity from restart file information.
    ///
    /// \param[in] segmentIDIndex The index of the segment ID in the IACN vector.
    /// Expected to be either LHSSegmentID or RHSSegmentID.
    ///
    /// \param[in] wellNameIndex The index of the well name in the ZACN vector.
    /// Expected to be either LHSWell or RHSWell.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \param[in] zacn Restart file string items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the segment level
    /// quantity.  Contains the well name and, if the segment ID is positive,
    /// the string representation of the segment ID.
    std::vector<std::string>
    segmentArgs(const Opm::RestartIO::Helpers::VectorItems::IACN::index segmentIDIndex,
                const Opm::RestartIO::Helpers::VectorItems::ZACN::index wellNameIndex,
                std::span<const int>                                    iacn,
                std::span<const std::string>                            zacn)
    {
        auto args = std::vector { zacn[wellNameIndex] };

        if (const auto segID = iacn[segmentIDIndex]; segID > 0) {
            args.push_back(fmt::format("{}", segID));
        }

        return args;
    }

    /// Derive string representation of an IJK location from restart file information.
    ///
    /// Common backend for block and connection level quantities.  The IJK location is
    /// represented by its indices in the IACN vector.
    ///
    /// \param[in] ijk The indices of the IJK location in the IACN vector.
    /// Expected to be either a block-level location like
    ///
    ///    { LHSBlockLocation_I, LHSBlockLocation_J, LHSBlockLocation_K },
    ///    { RHSBlockLocation_I, RHSBlockLocation_J, RHSBlockLocation_K }
    ///
    /// or a connection-level location like
    ///
    ///    { LHSConnLocation_I, LHSConnLocation_J, LHSConnLocation_K },
    ///    { RHSConnLocation_I, RHSConnLocation_J, RHSConnLocation_K }
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \return A vector of strings representing the IJK location.  Contains the string
    /// representations of the IJK indices, with "0" for non-positive values.
    std::vector<std::string>
    ijkArgs(const std::array<Opm::RestartIO::Helpers::VectorItems::IACN::index, 3>& ijk,
            std::span<const int>                                                    iacn)
    {
        auto args = std::vector<std::string> {};

        for (const auto& ijkIndex : ijk) {
            if (const auto loc = iacn[ijkIndex]; loc > 0) {
                args.push_back(fmt::format("{}", loc));
            }
            else {
                args.push_back("0");
            }
        }

        return args;
    }

    /// @brief Construct argument vector for a connection level quantity from restart file information.
    ///
    /// \param[in] connIndices The indices of the connection location in the IACN vector.
    /// Expected to be either { LHSConnLocation_I, LHSConnLocation_J, LHSConnLocation_K }
    /// or { RHSConnLocation_I, RHSConnLocation_J, RHSConnLocation_K }.
    ///
    /// \param[in] wellNameIndex The index of the well name in the ZACN vector.
    /// Expected to be either LHSWell or RHSWell.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \param[in] zacn Restart file string items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the connection level
    /// quantity.  Contains the well name and the string representations of the
    /// connection location indices.
    std::vector<std::string>
    connectionArgs(const std::array<Opm::RestartIO::Helpers::VectorItems::IACN::index, 3>& connIndices,
                   const Opm::RestartIO::Helpers::VectorItems::ZACN::index                 wellNameIndex,
                   std::span<const int>                                                    iacn,
                   std::span<const std::string>                                            zacn)
    {
        auto args = ijkArgs(connIndices, iacn);
        args.insert(args.begin(), zacn[wellNameIndex]);

        return args;
    }

    // --------------------------------------------------------------------------

    /// Construct argument vector for a left-hand side region from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the left-hand side region.
    std::vector<std::string>
    lhsRegionArgs(std::span<const int> iacn)
    {
        return regionArgs(Opm::RestartIO::Helpers::VectorItems::IACN::index::LHSRegionID, iacn);
    }

    /// Construct argument vector for a left-hand side segment from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \param[in] zacn Restart file string items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the left-hand side segment.
    std::vector<std::string>
    lhsSegmentArgs(std::span<const int>         iacn,
                   std::span<const std::string> zacn)
    {
        return segmentArgs(Opm::RestartIO::Helpers::VectorItems::IACN::index::LHSSegmentID,
                           Opm::RestartIO::Helpers::VectorItems::ZACN::index::LHSWell,
                           iacn, zacn);
    }

    /// Construct argument vector for a left-hand side connection from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \param[in] zacn Restart file string items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the left-hand side connection.
    std::vector<std::string>
    lhsConnectionArgs(std::span<const int>         iacn,
                      std::span<const std::string> zacn)
    {
        using IIndex = Opm::RestartIO::Helpers::VectorItems::IACN::index;
        using ZIndex = Opm::RestartIO::Helpers::VectorItems::ZACN::index;

        return connectionArgs({ IIndex::LHSConnLocation_I,
                                IIndex::LHSConnLocation_J,
                                IIndex::LHSConnLocation_K },
                              ZIndex::LHSWell, iacn, zacn);
    }

    /// Construct argument vector for a left-hand side block from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the left-hand side block.
    std::vector<std::string>
    lhsBlockArgs(std::span<const int> iacn)
    {
        using IIndex = Opm::RestartIO::Helpers::VectorItems::IACN::index;

        return ijkArgs({ IIndex::LHSBlockLocation_I,
                         IIndex::LHSBlockLocation_J,
                         IIndex::LHSBlockLocation_K },
                       iacn);
    }

    // --------------------------------------------------------------------------

    /// Construct argument vector for a right-hand side region from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the right-hand side region.
    std::vector<std::string>
    rhsRegionArgs(std::span<const int> iacn)
    {
        return regionArgs(Opm::RestartIO::Helpers::VectorItems::IACN::index::RHSRegionID, iacn);
    }

    /// Construct argument vector for a right-hand side segment from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \param[in] zacn Restart file string items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the right-hand side segment.
    std::vector<std::string>
    rhsSegmentArgs(std::span<const int>         iacn,
                   std::span<const std::string> zacn)
    {
        return segmentArgs(Opm::RestartIO::Helpers::VectorItems::IACN::index::RHSSegmentID,
                           Opm::RestartIO::Helpers::VectorItems::ZACN::index::RHSWell,
                           iacn, zacn);
    }

    /// Construct argument vector for a right-hand side connection from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \param[in] zacn Restart file string items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the right-hand side connection.
    std::vector<std::string>
    rhsConnectionArgs(std::span<const int>         iacn,
                      std::span<const std::string> zacn)
    {
        using IIndex = Opm::RestartIO::Helpers::VectorItems::IACN::index;
        using ZIndex = Opm::RestartIO::Helpers::VectorItems::ZACN::index;

        return connectionArgs({ IIndex::RHSConnLocation_I,
                                IIndex::RHSConnLocation_J,
                                IIndex::RHSConnLocation_K },
                              ZIndex::RHSWell, iacn, zacn);
    }

    /// Construct argument vector for a right-hand side block from restart file information.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \return A vector of strings representing the arguments for the right-hand side block.
    std::vector<std::string>
    rhsBlockArgs(std::span<const int> iacn)
    {
        using IIndex = Opm::RestartIO::Helpers::VectorItems::IACN::index;

        return ijkArgs({ IIndex::RHSBlockLocation_I,
                         IIndex::RHSBlockLocation_J,
                         IIndex::RHSBlockLocation_K },
                       iacn);
    }

    /// Construct a right-hand side quantity for a constant value from restart file information.
    ///
    /// The constant is expected to be a numeric value or a named month.  If the
    /// left hand side quantity is a Month and the numeric value is an integer,
    /// the corresponding month name is returned.  Otherwise, the numeric value
    /// is returned as a string.
    ///
    /// \param[in] iacn Restart file integer items for a single action condition.
    ///
    /// \param[in] sacn Restart file double items for a single action condition.
    ///
    /// \return A quantity representing the right-hand side constant value.
    Opm::RestartIO::RstAction::Quantity
    rhsConstant(std::span<const int>    iacn,
                std::span<const double> sacn)
    {
        using IIndex = Opm::RestartIO::Helpers::VectorItems::IACN::index;
        using QType = Opm::RestartIO::Helpers::VectorItems::IACN::Value::QuantityType;
        using SIndex = Opm::RestartIO::Helpers::VectorItems::SACN::index;

        if ((iacn[IIndex::LHSQuantityType] == QType::Month) &&
            isNumericInteger(sacn[SIndex::RHSValue0]))
        {
            const auto& monthNames = Opm::TimeService::eclipseMonthNames();

            auto monthPos = monthNames.find(static_cast<int>(sacn[SIndex::RHSValue0]));
            if (monthPos != monthNames.end()) {
                return Opm::RestartIO::RstAction::Quantity{monthPos->second};
            }
        }

        return Opm::RestartIO::RstAction::
            Quantity{fmt::format("{}", sacn[SIndex::RHSValue0])};
    }

} // Anonymous namespace

namespace Opm::RestartIO {

    namespace IACN = Helpers::VectorItems::IACN;
    namespace SACN = Helpers::VectorItems::SACN;
    namespace ZACN = Helpers::VectorItems::ZACN;

    RstAction::Quantity::Quantity(const std::string& q)
        : quantity_ { q }
    {}

    RstAction::Condition::Condition(std::span<const int>         iacn,
                                    std::span<const double>      sacn,
                                    std::span<const std::string> zacn)
        : logic  { Action::logic_from_int(iacn[IACN::TerminalLogic]) }
        , cmp_op { Action::comparator_from_int(iacn[IACN::Comparator]) }
    {
        if (iacn.size() != IACN::ConditionSize) {
            throw std::invalid_argument {
                fmt::format("Expected IACN vector "
                            "of size {}, but got {}",
                             IACN::ConditionSize, iacn.size())
            };
        }

        if (sacn.size() != SACN::ConditionSize) {
            throw std::invalid_argument {
                fmt::format("Expected SACN vector "
                            "of size {}, but got {}",
                             SACN::ConditionSize, sacn.size())
            };
        }

        if (zacn.size() != ZACN::ConditionSize) {
            throw std::invalid_argument {
                fmt::format("Expected ZACN vector "
                            "of size {}, but got {}",
                             ZACN::ConditionSize, zacn.size())
            };
        }

        this->restoreLHSQuantity(iacn, zacn);
        this->restoreRHSQuantity(iacn, sacn, zacn);

        this->left_paren = iacn[IACN::Paren] == IACN::Value::Open;
        this->right_paren = iacn[IACN::Paren] == IACN::Value::Close;
    }

bool
RstAction::Condition::valid(std::span<const int>         iacn,
                            std::span<const std::string> zacn)
{
    auto type_index = IACN::LHSQuantityType;
    if (iacn[type_index] == IACN::Value::Day) {
        return true;
    }

    if (iacn[type_index] == IACN::Value::Month) {
        return true;
    }

    if (iacn[type_index] == IACN::Value::Year) {
        return true;
    }

    auto str_quantity = trim_copy(zacn[ZACN::LHSQuantity]);
    return !str_quantity.empty();
}

void
RstAction::Condition::restoreLHSQuantity(std::span<const int>         iacn,
                                         std::span<const std::string> zacn)
{
    using IIndex = IACN::index;
    using ZIndex = ZACN::index;
    using QType = IACN::Value::QuantityType;

    switch (iacn[IIndex::LHSQuantityType]) {
    case QType::Field:
        this->lhs = RstAction::Quantity{zacn[ZIndex::LHSQuantity]};
        break;

    case QType::Well:
        this->lhs = RstAction::Quantity{zacn[ZIndex::LHSQuantity]};
        this->lhs.args_ = { zacn[ZIndex::LHSWell] };
        break;

    case QType::Group:
        this->lhs = RstAction::Quantity{zacn[ZIndex::LHSQuantity]};
        this->lhs.args_ = { zacn[ZIndex::LHSGroup] };
        break;

    case QType::Region:
        this->lhs = RstAction::Quantity{zacn[ZIndex::LHSQuantity]};
        this->lhs.args_ = lhsRegionArgs(iacn);
        break;

    case QType::Segment:
        this->lhs = RstAction::Quantity{zacn[ZIndex::LHSQuantity]};
        this->lhs.args_ = lhsSegmentArgs(iacn, zacn);
        break;

    case QType::Connection:
        this->lhs = RstAction::Quantity{zacn[ZIndex::LHSQuantity]};
        this->lhs.args_ = lhsConnectionArgs(iacn, zacn);
        break;

    case QType::Day:
        this->lhs = RstAction::Quantity{"DAY"};
        break;

    case QType::Month:
        this->lhs = RstAction::Quantity{"MNTH"};
        break;

    case QType::Year:
        this->lhs = RstAction::Quantity{"YEAR"};
        break;

    case QType::Block:
        this->lhs = RstAction::Quantity {zacn[ZIndex::LHSQuantity]};
        this->lhs.args_ = lhsBlockArgs(iacn);
        break;
    }
}

void
RstAction::Condition::restoreRHSQuantity(std::span<const int>         iacn,
                                         std::span<const double>      sacn,
                                         std::span<const std::string> zacn)
{
    using IIndex = IACN::index;
    using SIndex = SACN::index;
    using ZIndex = ZACN::index;
    using QType = IACN::Value::QuantityType;

    switch (iacn[IIndex::RHSQuantityType]) {
    case QType::Field:
        this->rhs = RstAction::Quantity{zacn[ZIndex::RHSQuantity]};
        break;

    case QType::Well:
        this->rhs = RstAction::Quantity{zacn[ZIndex::RHSQuantity]};
        this->rhs.args_ = { zacn[ZIndex::RHSWell] };
        break;

    case QType::Group:
        this->rhs = RstAction::Quantity{zacn[ZIndex::RHSQuantity]};
        this->rhs.args_ = { zacn[ZIndex::RHSGroup] };
        break;

    case QType::Region:
        this->rhs = RstAction::Quantity{zacn[ZIndex::RHSQuantity]};
        this->rhs.args_ = rhsRegionArgs(iacn);
        break;

    case QType::Segment:
        this->rhs = RstAction::Quantity{zacn[ZIndex::RHSQuantity]};
        this->rhs.args_ = rhsSegmentArgs(iacn, zacn);
        break;

    case QType::Connection:
        this->rhs = RstAction::Quantity{zacn[ZIndex::RHSQuantity]};
        this->rhs.args_ = rhsConnectionArgs(iacn, zacn);
        break;

    case QType::Const:
        this->rhs = rhsConstant(iacn, sacn);
        break;

    case QType::Block:
        this->rhs = RstAction::Quantity {zacn[ZIndex::RHSQuantity]};
        this->rhs.args_ = rhsBlockArgs(iacn);
        break;
    }
}

// --------------------------------------------------------------------------

RstAction::RstAction(const std::string& name_arg,
                     const int max_run_arg,
                     const int run_count_arg,
                     const double min_wait_arg,
                     const std::time_t start_time_arg,
                     const std::time_t last_run_arg,
                     std::vector<RstAction::Condition>&& conditions_arg)
    : name       { name_arg }
    , max_run    { max_run_arg }
    , run_count  { run_count_arg }
    , min_wait   { min_wait_arg }
    , start_time { start_time_arg }
    , last_run   { run_count_arg > 0 ? std::optional{last_run_arg} : std::nullopt }
    , conditions { std::move(conditions_arg) }
{}

std::vector<std::string>
RstAction::Condition::tokens() const
{
    auto tokens = std::vector<std::string> {};
    if (this->left_paren) {
        tokens.push_back("(");
    }

    captureQuantityTokens(this->lhs, tokens);
    tokens.push_back(Action::comparator_as_string(this->cmp_op));
    captureQuantityTokens(this->rhs, tokens);

    if (this->right_paren) {
        tokens.push_back(")");
    }

    switch (this->logic) {
    case Action::Logical::AND:
        tokens.push_back("AND");
        break;

    case Action::Logical::OR:
        tokens.push_back("OR");
        break;

    case Action::Logical::END:
        break;
    }

    return tokens;
}

} // namespace Opm::RestartIO
