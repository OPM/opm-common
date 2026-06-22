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

#ifndef RST_ACTIONX
#define RST_ACTIONX

#include <opm/output/eclipse/VectorItems/action.hpp>

#include <opm/input/eclipse/Schedule/Action/Enums.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <ctime>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Opm::RestartIO {

/// ACTIONX keyword constructed from restart file information.
struct RstAction
{
    class Condition;

    /// Restart file representation of an action condition quantity.
    ///
    /// This is typically a summary vector, e.g., a field level quantity
    /// such as FOPT, a group level quantity such as GOPR for a particular
    /// group, or a well level quantity such as WWCT for one or more wells.
    class Quantity
    {
    public:
        /// Default constructor.
        ///
        /// Mostly to support deferred initialization, e.g., for serialization.
        Quantity() = default;

        /// Construct a Quantity with the given string.
        ///
        /// \param[in] quantity Quantity name or string representation
        /// of a numeric constant or named month.
        explicit Quantity(const std::string& quantity);

        /// Retrieve the quantity name.
        ///
        /// \return The quantity name or a string representation
        /// of a numeric constant or named month.
        const std::string& quantity() const { return this->quantity_; }

        /// Retrieve the quantity arguments.
        ///
        /// \return The quantity arguments.  Empty if there are no additional
        /// arguments such as for field level quantities, date keywords or
        /// numeric constants.  Otherwise, contains e.g., well or group name,
        /// segment numbers, region IDs, or cell IJK tuples.
        const std::vector<std::string>& args() const { return this->args_; }

        friend class Condition;

    private:
        /// Action condition quantity name.
        std::string quantity_ {};

        /// Action condition quantity arguments.
        ///
        /// Populated by Condition.
        std::vector<std::string> args_ {};
    };

    /// Restart file representation of an action condition.
    ///
    /// This is a single condition in an ACTIONX keyword.  It consists
    /// of a left hand side quantity, a comparison operator, and a right
    /// hand side quantity.  It may additionally include a logical operator
    /// by which to combine it with the next condition in a sequence
    /// of conditions.
    class Condition
    {
    public:
        /// Check if the given restart file information represents a valid
        /// action condition.
        ///
        /// \param[in] iacn Integer condition items, notably including
        /// the quantity types and comparison operator.
        ///
        /// \param[in] zacn String condition items, notably including
        /// the left and right hand side quantity names.
        ///
        /// \return True if the given restart file information represents
        /// a valid action condition, false otherwise.
        static bool
        valid(std::span<const int>         iacn,
              std::span<const std::string> zacn);

        /// Construct a Condition from restart file information.
        ///
        /// \param[in] iacn Integer condition items, notably including
        /// the quantity types and comparison operator. Must have size
        /// IACN::ConditionSize.
        ///
        /// \param[in] sacn Double condition items, notably including
        /// the right hand side numeric values. Must have size
        /// SACN::ConditionSize.
        ///
        /// \param[in] zacn String condition items, notably including
        /// the left and right hand side quantity names. Must have size
        /// ZACN::ConditionSize.
        Condition(std::span<const int>         iacn,
                  std::span<const double>      sacn,
                  std::span<const std::string> zacn);

        /// Logical operator of the condition.
        Action::Logical logic;

        /// Comparison operator of the condition.
        Action::Comparator cmp_op;

        /// Left hand side quantity of the condition.
        ///
        /// This is typically a summary vector, e.g., a field level quantity
        /// such as FOPT, a group level quantity such as GOPR for a particular
        /// group, or a well level quantity such as WWCT for one or more wells.
        Quantity lhs;

        /// Right hand side quantity of the condition.
        ///
        /// This is typically a constant or a specific summary vector.
        Quantity rhs;

        /// Whether or not the condition starts with a left parenthesis.
        bool left_paren {false};

        /// Whether or not the condition includes a right parenthesis.
        bool right_paren {false};

        /// Canonicalised string tokens comprising the condition.
        ///
        /// No leading or trailing whitespace for any token.
        ///
        /// \return Vector of string tokens.
        std::vector<std::string> tokens() const;

    private:
        /// Restore the left hand side quantity from restart file information.
        ///
        /// \param[in] iacn Integer condition items, notably including the
        /// quantity types and comparison operator.
        ///
        /// \param[in] zacn String condition items, notably including the
        /// left and right hand side quantity names.
        void restoreLHSQuantity(std::span<const int>         iacn,
                                std::span<const std::string> zacn);

        /// Restore the right hand side quantity from restart file information.
        ///
        /// \param[in] iacn Integer condition items, notably including the
        /// quantity types and comparison operator.
        ///
        /// \param[in] sacn Double condition items, notably including
        /// constant values.
        ///
        /// \param[in] zacn String condition items, notably including
        /// the left and right hand side quantity names.
        void restoreRHSQuantity(std::span<const int>         iacn,
                                std::span<const double>      sacn,
                                std::span<const std::string> zacn);
    };

    /// Construct a new RstAction object from restart file information.
    ///
    /// \param[in] name_arg Action name.
    ///
    /// \param[in] max_run_arg Maximum number of times the action can be run.
    ///
    /// \param[in] run_count_arg Number of times the action has been run.
    ///
    /// \param[in] min_wait_arg Minimum wait time between action runs.
    ///
    /// \param[in] start_time Start time of the action.
    ///
    /// \param[in] last_run Last run time of the action.
    ///
    /// \param[in] conditions_arg Conditions associated with the action.
    RstAction(const std::string& name_arg,
              int max_run_arg,
              int run_count_arg,
              double min_wait_arg,
              std::time_t start_time,
              std::time_t last_run,
              std::vector<Condition>&& conditions_arg);

    /// Action name.
    std::string name;

    /// Maximum number of times the action can be run.
    int max_run;

    /// Number of times the action has been run.
    int run_count;

    /// Minimum wait time between action runs.
    double min_wait;

    /// Start time of the action.
    std::time_t start_time;

    /// Last run time of the action.
    ///
    /// Nullopt if the action has not yet run.
    std::optional<std::time_t> last_run;

    /// Conditions associated with the action.
    std::vector<Condition> conditions;

    /// Action block keywords associated with the action.
    std::vector<DeckKeyword> keywords;
};

} // namespace Opm::RestartIO

#endif // RST_ACTIONX
