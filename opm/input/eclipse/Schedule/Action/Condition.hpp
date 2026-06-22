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

#ifndef ACTIONX_CONDITION_HPP
#define ACTIONX_CONDITION_HPP

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <opm/io/eclipse/rst/action.hpp>

#include <opm/input/eclipse/Schedule/Action/Enums.hpp>

#include <optional>
#include <string>
#include <vector>

namespace Opm::Action {

class Condition;

} // namespace Opm::Action

namespace Opm::Action {

/// Restart file representation of an action condition quantity.
///
/// This is typically a summary vector, e.g., a field level quantity
/// such as FOPT, a group level quantity such as GOPR for a particular
/// group, or a well level quantity such as WWCT for one or more wells.
///
/// Additional types include segment and region level quantities,
/// as well as numerical constants or named months.
class Quantity
{
public:
    /// Construct a default quantity.
    ///
    /// Mostly to support deferred initialization, e.g., for serialization.
    Quantity() = default;

    /// Construct a quantity from a quantity name.
    ///
    /// This is typically a summary vector name.
    ///
    /// \param[in] arg The quantity name.
    explicit Quantity(const std::string& arg);

    /// Construct a quantity from a restart action quantity.
    ///
    /// \param[in] rst_quantity The restart action quantity.  Expected
    /// to include all requisite additional arguments such as group
    /// names or segment numbers.
    explicit Quantity(const RestartIO::RstAction::Quantity& rst_quantity);

    /// Create a serialization test object.
    static Quantity serializationTestObject();

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

    /// Integer type classification of the quantity.
    ///
    /// Expected to be one of the
    ///
    ///    RestartIO::Helpers::VectorItems::IACN::Value::QuantityType
    ///
    /// enumerators defined in VectorItems/action.hpp.
    ///
    /// \return The integer type classification of the quantity.
    int int_type() const;

    /// Equality predicate.
    ///
    /// \param[in] that Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p that.
    bool operator==(const Quantity& that) const
    {
        return (this->quantity_ == that.quantity_)
            && (this->args_     == that.args_)
            && (this->type_     == that.type_)
            ;
    }

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->quantity_);
        serializer(this->args_);
        serializer(this->type_);
    }

    friend class Condition;

private:
    /// Action condition quantity name.
    std::string quantity_{};

    /// Action condition quantity arguments.
    std::vector<std::string> args_{};

    /// Action condition quantity type.
    ///
    /// Populated in finalise().
    std::optional<int> type_{};

    /// Determine the type of the quantity based on its name and arguments.
    ///
    /// \return The integer type classification of the quantity.
    int type() const;

    /// Check if the quantity is a numeric constant.
    ///
    /// \return True if the quantity is a numeric constant, false otherwise.
    bool isNumeric() const;

    /// Check if the quantity represents a day.
    ///
    /// \return True if quantity_ == "DAY", false otherwise.
    bool isDay() const;

    /// Check if the quantity represents a month.
    ///
    /// \return True if quantity_ == "MNTH" or quantity_ == "MONTH", false otherwise.
    bool isMonth() const;

    /// Check if the quantity represents a month name.
    ///
    /// \return True if quantity_ is one the canonical month name
    /// abbreviations recognized by OPM, false otherwise.
    bool isMonthName() const;

    /// Check if the quantity represents a year.
    ///
    /// \return True if quantity_ == "YEAR", false otherwise.
    bool isYear() const;

    /// Add an argument to the quantity.
    ///
    /// Invoked by the Condition constructor as it parses the quantity
    /// and its arguments.  The arguments are stored in the order they
    /// are encountered, and the finalise() method is invoked after
    /// all arguments have been added.
    ///
    /// \param[in] arg The argument to be added.
    void add_arg(const std::string& arg);

    /// Finalise the quantity.
    ///
    /// Invoked by the Condition constructor after the quantity has been
    /// fully constructed and all arguments have been added.  Determines
    /// the type of the quantity based on its name and arguments, and
    /// performs any necessary validation or normalisation of the quantity
    /// and its arguments.
    void finalise();

    /// Finalise a region quantity type.
    ///
    /// This is a helper function called by finalise().
    ///
    /// Ensures that the quantity keyword includes the region set name and
    /// that the only argument is a region ID.
    void finaliseRegion();
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
    /// Default constructor.
    ///
    /// Mostly to support deferred initialization, e.g., for serialization.
    Condition() = default;

    /// Construct a Condition from restart file information.
    ///
    /// \param[in] rst_condition The restart file condition to construct from.
    explicit Condition(const RestartIO::RstAction::Condition& rst_condition);

    /// Construct a Condition from SCHEDULE section information.
    ///
    /// \param[in] tokens The raw string tokens comprising the condition.
    /// Each token is assumed to be free of leading and trailing whitespace,
    /// and to have been split on whitespace.
    ///
    /// \param[in] location The location of the pertinent action
    /// keyword in the input deck.  Needed for error reporting if
    /// the condition cannot be parsed.
    Condition(const std::vector<std::string>& tokens,
              const KeywordLocation&          location);

    /// Left hand side quantity of the condition.
    ///
    /// This is typically a summary vector, e.g., a field level quantity
    /// such as FOPT, a group level quantity such as GOPR for a particular
    /// group, or a well level quantity such as WWCT for one or more wells.
    Quantity lhs{};

    /// Right hand side quantity of the condition.
    ///
    /// This is typically a constant or a specific summary vector.
    Quantity rhs{};

    /// Logical operator of the condition.
    ///
    /// Operator by which to combine this condition with the next condition
    /// in a sequence of conditions.  The default value is Logical::END,
    /// which indicates that this is the last or only condition in a sequence
    /// of conditions.  Other possibilities are Logical::AND and Logical::OR.
    Logical logic { Logical::END };

    /// Comparison operator of the condition.
    ///
    /// This is expected to be a regular boolean comparison operator
    /// such as >, <, >=, <=, ==, or !=.
    Comparator cmp { Comparator::INVALID };

    /// Whether or not the condition starts with a left parenthesis.
    bool left_paren { false };

    /// Whether or not the condition includes a right parenthesis.
    bool right_paren { false };

    /// String representation of the comparison operator.
    ///
    /// Needed for restart file output.
    std::string cmp_string{};

    /// Compute restart file integer representation of the logical operator.
    ///
    /// \return Integer representation of the condition's logical operator.
    int logic_as_int() const;

    /// Compute restart file integer representation of the comparison operator.
    ///
    /// \return Integer representation of the condition's comparison operator.
    int comparator_as_int() const;

    /// Compute restart file integer representation of the parentheses.
    ///
    /// \return Integer representation of the condition's parentheses level.
    int paren_as_int() const;

    /// Whether or not this condition increases the grouping level
    /// of the condition sequence.
    ///
    /// \return True if the condition starts with a left parenthesis
    /// and does *not* include a right parenthesis, false otherwise.
    bool open_paren() const;

    /// Whether or not this condition decreases the grouping level
    /// of the condition sequence.
    ///
    /// \return True if the condition does *not* start with a left
    /// parenthesis but does include a right parenthesis, false otherwise.
    bool close_paren() const;

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const Condition& data) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(lhs);
        serializer(rhs);
        serializer(logic);
        serializer(cmp);
        serializer(cmp_string);
        serializer(left_paren);
        serializer(right_paren);
    }
};

} // namespace Opm::Action

#endif // ACTIONX_CONDITION_HPP
