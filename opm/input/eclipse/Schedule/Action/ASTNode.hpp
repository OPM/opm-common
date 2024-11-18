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

#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include <opm/input/eclipse/Schedule/Action/ActionValue.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace Opm::Action {
    class Context;
} // namespace Opm::Action

namespace Opm::Action {

/// Node in ACTIONX condition expression abstract syntax tree
///
/// Might for instance represent the conjunction ('AND') in a condition of
/// the form
///
///    FGOR > 432.1 AND /
///    (WMCTL 'PROD*' = 1 OR /
///     GWIR < GUWIRMIN) /
///
/// In this case, the direct children would be the 'FGOR' condition and the
/// grouped disjunction '(WMCTL OR GWIR)'.
class ASTNode
{
public:
    /// Default constructor
    ///
    /// Creates an AST node representing an error condition.  Exists mostly
    /// for serialisation purposes.
    ASTNode();

    /// Constructor
    ///
    /// Creates an AST node representing an expression, a relation, a
    /// grouping, or a value.
    ///
    /// \param[in] type_arg Kind of AST node.
    explicit ASTNode(TokenType type_arg);

    /// Constructor
    ///
    /// Creates a leaf-level AST node representing a numeric value.
    ///
    /// \param[in] value Numeric value in leaf-level AST node.
    explicit ASTNode(double value);

    /// Constructor
    ///
    /// Creates an AST node representing an expression, a relation, a
    /// grouping, or a value.  Records any applicable function name along
    /// with arguments--e.g., a list of well names--upon which to invoke the
    /// function.
    ///
    /// \param[in] type_arg Kind of AST node.
    ///
    /// \param[in] func_type_arg Function category of this AST node.
    ///
    /// \param[in] func_arg Which function (i.e., summary vector or UDQ) to
    /// evaluate at this AST node.  Pass an empty string if there is no
    /// function.  Common examples include FGOR, WOPR, or GGLR.
    ///
    /// \param[in] arg_list_arg Any additional arguments to the \p func_arg
    /// function.  Might for instance be a list of well or group names.
    explicit ASTNode(TokenType type_arg,
                     FuncType func_type_arg,
                     std::string_view func_arg,
                     const std::vector<std::string>& arg_list_arg);

    /// Kind of AST node.
    TokenType type;

    /// Function category of this AST node.
    FuncType func_type;

    /// Which function to evaluate at this AST node.  Empty string for no
    /// function.  Typically a summary vector name otherwise.
    std::string func;

    /// Create a serialisation test object.
    static ASTNode serializationTestObject();

    /// Parent a node to current AST node.
    ///
    /// Note: The order of add_child() calls may matter.  For instance, if
    /// the current node represents a binary operator such addition or a
    /// logical comparison--see the \c type member--then child nodes will be
    /// treated left-to-right in the order of add_child() calls.  This is
    /// especially important for comparisons like '<', subtraction, or
    /// division.
    ///
    /// \param[in] child Expression node which will be parented to the
    /// current AST node object.
    void add_child(ASTNode&& child);

    /// Evaluate logical expression
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Expression value.  Any wells for which the expression is
    /// true will be included in the result set.
    Result eval(const Context& context) const;

    /// Export all summary vectors needed to compute values for the
    /// current collection of user defined quantities.
    ///
    /// \param[in,out] required_summary Named summary vectors.  Upon
    /// completion, any additional summary vectors needed to evaluate the
    /// current condition will be included in this set.
    void required_summary(std::unordered_set<std::string>& required_summary) const;

    /// Equality predicate.
    ///
    /// \param[in] that Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p other.
    bool operator==(const ASTNode& that) const;

    /// Number of child nodes of this AST node.
    std::size_t size() const;

    /// Query whether or not this AST node has any child nodes.
    bool empty() const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(type);
        serializer(func_type);
        serializer(func);
        serializer(arg_list);
        serializer(number);
        serializer(children);
    }

private:
    // Note: data member order here is dictated by initialisation list in
    // four-argument constructor.

    /// Additional arguments upon which to invoke the \c func function.
    /// Might, for instance, be a list of well or group names.
    std::vector<std::string> arg_list{};

    /// Numeric value of a scalar AST node.
    double number {0.0};

    // Note: a data member of type std::vector<ASTNode> inside class ASTNode
    // is well defined in C++17 or later, but may look surprising to the
    // uninitiated.

    /// Child nodes of this AST node.
    std::vector<ASTNode> children{};

    /// Evaluate a conjunction/disjunction ('AND'/'OR') node.
    ///
    /// This is a top-level entry point for eval().
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Expression value.  Any wells for which the expression is
    /// true will be included in the result set.
    Result evalLogicalOperation(const Context& context) const;

    /// Evaluate a leaf-level comparison ('<', '=', '>=' &c) node.
    ///
    /// This is a top-level entry point for eval() when the node itself is a
    /// leaf-level condition.
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Expression value.  Any wells for which the expression is
    /// true will be included in the result set.
    Result evalComparison(const Context& context) const;

    /// Compute numeric value of leaf node and collect associated entities.
    ///
    /// Must only be called for leaf nodes--i.e., nodes for which there are
    /// no child nodes.
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Numeric value of a function applied to a sequence of
    /// arguments.  Also includes names of any wells mentioned in the
    /// function argument list.
    Value nodeValue(const Context& context) const;

    /// Compute numeric value of leaf node when argument list is a name
    /// pattern.
    ///
    /// Must only be called for leaf nodes--i.e., nodes for which there are
    /// no child nodes.
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Numeric value of a function applied to a sequence of
    /// arguments.  Also includes names of any wells matching the pattern in
    /// the function argument list.
    Value evalListExpression(const Context& context) const;

    /// Compute numeric value of leaf node when argument list is a single
    /// name.
    ///
    /// Must only be called for leaf nodes--i.e., nodes for which there are
    /// no child nodes.
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Numeric value of a function applied to a sequence of
    /// arguments.  Also includes the name the well mentioned in the
    /// function argument list.
    Value evalScalarExpression(const Context& context) const;

    /// Compute numeric value of leaf node when argument list is a well name
    /// pattern.
    ///
    /// Must only be called for leaf nodes--i.e., nodes for which there are
    /// no child nodes.
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Numeric value of a function applied to a sequence of
    /// arguments.  Also includes names of any wells matching the pattern in
    /// the function argument list.
    Value evalWellExpression(const Context& context) const;

    /// Compute list of well names matching a name pattern.
    ///
    /// \param[in] context Current summary vector values and well query
    /// object.
    ///
    /// \return Names of all wells matching the well name pattern in \code
    /// arg_list.front() \endcode, irrespective of whether the pattern is a
    /// well template, a well list, or a well list template.
    std::vector<std::string> getWellList(const Context& context) const;

    /// Query whether or not the front of the function argument list (\code
    /// arg_list.front() \endcode) is a name pattern.
    bool argListIsPattern() const;

    /// Query whether or not the front of the function argument list (\code
    /// arg_list.front() \endcode) is the name of a well list or a well list
    /// template (pattern).
    bool argListIsWellList() const;
};

} // namespace Opm::Action

#endif // ASTNODE_HPP
