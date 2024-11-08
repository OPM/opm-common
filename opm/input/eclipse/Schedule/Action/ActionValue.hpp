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

#ifndef ACTION_VALUE_HPP
#define ACTION_VALUE_HPP

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Opm::Action {

/// Lexical token in ACTIONX condition expression
enum class TokenType
{
    /// Token is a literal number (e.g., 42 or -2.718e2)
    number, // 0

    /// Token is an expression--e.g., a function invocation or an arithmetic
    /// combination of functions and values.
    ecl_expr, // 1

    /// Token is an opening parenthesis (i.e. "(")
    open_paren, // 2

    /// Token is a closing parenthesis (i.e., ")")
    close_paren, // 3

    /// Token is a greater-than operator (i.e., ">" or ".GT.")
    op_gt, // 4

    /// Token is a greater-than-or-equal-to operator (i.e., ">=" or ".GE.")
    op_ge, // 5

    /// Token is a less-than operator (i.e., "<" or ".LT.")
    op_lt, // 6

    /// Token is a less-than-or-equal-to operator (i.e., "<=" or ".LE.")
    op_le, // 7

    /// Token is an equality operator (i.e., "=" or ".EQ.")
    op_eq, // 8

    /// Token is a not-equal operator (i.e., "!=" or ".NE.")
    op_ne, // 9

    /// Token is the logical conjunction ("AND")
    op_and, // 10

    /// Token is the logical disjunction ("OR")
    op_or, // 11

    /// Token is the end-of-record (forward slash, "/")
    end, // 12

    /// Parse error state.
    error, // 13
};

/// Function category of an ACTION condition sub-expression
enum class FuncType
{
    /// No applicable function
    none, // 0

    /// Function is derived time quantity--e.g., the DAY or the YEAR--of the
    /// current simulated TIME.
    time, // 1

    /// Function is the month ("MNTH") of the current simulated TIME.
    time_month, // 2

    /// Function applies to the region level (e.g., ROPR)
    region, // 3

    /// Function applies to the field level (e.g., FGOR)
    field, // 4

    /// Function applies to the group level (e.g., GOPRS)
    group, // 5

    /// Function applies to the well level (e.g., WWCT)
    well, // 6

    /// Function applies to the segment level (e.g., SOFR)
    well_segment,  // 7

    /// Function applies to the well connection level (e.g., CPR)
    well_connection, // 8

    /// Function applies to wells in an LGR.  Not really supported.
    Well_lgr, // 9

    /// Function applies to the aquifer level (e.g., AAQP)
    aquifer, // 10

    /// Function applies to the block/cell level (e.g., BDENO)
    block, // 11
};

/// Numeric value of an AST sub-expression
class Value
{
public:
    /// Default constructor.
    ///
    /// Resulting object is meaningful only if calling code later invokes
    /// the add_well() member function.
    Value() = default;

    /// Constructor.
    ///
    /// Creates a scalar Value object.
    ///
    /// \param[in] value Numeric value of scalar Value object.
    explicit Value(double value);

    /// Constructor.
    ///
    /// Creates a non-scalar Value object associated to a single well.
    Value(std::string_view wname, double value);

    /// Compare current Value to another Value.
    ///
    /// \param[in] op Comparison operator.  Must be one of
    ///     * TokenType::op_eq (==)
    ///     * TokenType::op_ge (>=)
    ///     * TokenType::op_le (<=)
    ///     * TokenType::op_ne (!=)
    ///     * TokenType::op_gt (>)
    ///     * TokenType::op_lt (<)
    ///   Function eval_cmp() will throw an exception of type
    ///   std::invalid_argument unless \p op is one of these operators.
    ///
    /// \param[in] rhs Value object against which \code *this \endcode will
    /// be compared through \p op.  Should be a scalar value.  The \p rhs
    /// object will be used on the right-hand side of the comparison
    /// operator while \code *this \endcode will be used on the left-hand
    /// side of \p op.  Function eval_cmp() will throw an exception of type
    /// std::invalid_argument if \p rhs is not a scalar Value object.
    ///
    /// \return Result of comparison "*this op rhs".  If \code *this
    /// \endcode is non-scalar, then any wells for which the comparison
    /// holds will be included in the result set.
    Result eval_cmp(TokenType op, const Value& rhs) const;

    /// Incorporate well level function value into Value object.
    ///
    /// Will throw an exception of type std::invalid_argument if \code *this
    /// \endcode was created as a scalar object.
    ///
    /// \param[in] well Named well for which to incorporate a function
    /// value.
    ///
    /// \param[in] value Numeric function value for \p well.
    void add_well(std::string_view well, double value);

    /// Retrieve scalar function value.
    ///
    /// Will throw an exception of type std::invalid_argument if \code *this
    /// \endcode was not created as a scalar object.
    double scalar() const;

private:
    /// Numeric value of scalar Value object.
    ///
    /// Meaningful only if \c is_scalar_ flag is true.
    double scalar_value_{};

    /// Whether or not current Value represents a scalar.
    double is_scalar_{false};

    /// Collection of function values associated to individual wells.
    std::vector<std::pair<std::string, double>> well_values_{};

    /// Compare current list of well values to another Value
    ///
    /// \param[in] op Comparison operator.
    ///
    /// \param[in] rhs Numerical value against which the current values will
    /// be compared through \p op.  The \p rhs value will be used on the
    /// right-hand side of the comparison operator while the current well
    /// values will be used on the left-hand side of \p op.
    ///
    /// \return Result of comparison "X op rhs" for all well values 'X'.
    /// True if the comparison holds for at least one well.  Any wells for
    /// which the comparison holds will be included in the result set.
    Result evalWellComparisons(TokenType op, double rhs) const;
};

} // namespace Opm::Action

#endif // ACTION_VALUE_HPP
