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

#include <opm/input/eclipse/Schedule/Action/ActionParser.hpp>

#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <algorithm>
#include <cassert>
#include <cstdlib>              // std::strtod()
#include <cstring>              // std::strlen()
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace {

/// Classify ACTIONX condition sub expression according to summary vector
/// category.
///
/// Mostly needed to aid condition evaluation later.
///
/// \param[in] arg Condition sub expression.  Expected to be the name of a
/// summary vector.
///
/// \return Summary function category of \p arg.
Opm::Action::FuncType functionType(const std::string& arg)
{
    using FuncType = Opm::Action::FuncType;

    if ((arg == "YEAR") || (arg == "DAY")) {
        return FuncType::time;
    }

    if (arg == "MNTH") { return FuncType::time_month; }

    using Cat = Opm::SummaryConfigNode::Category;

    switch (Opm::parseKeywordCategory(arg)) {
    case Cat::Aquifer:    return FuncType::aquifer;
    case Cat::Well:       return FuncType::well;
    case Cat::Group:      return FuncType::group;
    case Cat::Connection: return FuncType::well_connection;
    case Cat::Region:     return FuncType::region;
    case Cat::Block:      return FuncType::block;
    case Cat::Segment:    return FuncType::well_segment;
    default:              return FuncType::none;
    }
}

/// Convert string to lower case characters.
///
/// \param[in] arg Input string.  Assumed to contain characters in the basic
/// execution character set only.
///
/// \return Lower case copy of input string \p arg.
std::string makeLowercase(const std::string& arg)
{
    std::string lower_arg = arg;

    std::for_each(lower_arg.begin(), lower_arg.end(),
                  [](char& c) {
                      c = std::tolower(static_cast<unsigned char>(c));
                  });

    return lower_arg;
}

/// Convert sequence of action condition textual tokens into an expression
/// tree suitable for evaluation.
///
/// Client code is expected to construct an object and then call the
/// buildParseTree() member function, typically within one or two
/// statements.
class ActionParser
{
public:
    /// Constructor.
    ///
    /// Initiates parsing process by grabbing a reference to a sequence of
    /// condition tokens.
    ///
    /// \param[in] tokens Sequence of concatenated condition string tokens
    /// of a single ACTIONX condition block from which to form an expression
    /// evaluation tree.  Newline characters and other whitespace assumed to
    /// be removed.
    explicit ActionParser(const std::vector<std::string>& tokens)
        : tokens_ { tokens }
    {}

    /// Form expression evaluation tree from internal token sequence.
    std::unique_ptr<Opm::Action::ASTNode> buildParseTree();

private:
    /// Expression parse tree node
    ///
    /// Associates a parse tree string token to a token type
    struct Node
    {
        /// Default constructor.
        Node() = default;

        /// Constructor.
        ///
        /// \param[in] type_arg Condition token type, e.g., expression or
        /// logical operator.
        ///
        /// \param[in] value_arg Condition token string value.
        explicit Node(const Opm::Action::TokenType type_arg,
                      const std::string& value_arg = "")
            : type (type_arg)
            , value(value_arg)
        {}

        /// Condition token type.
        Opm::Action::TokenType type { Opm::Action::TokenType::error };

        /// Condition token string value.
        std::string value{};
    };

    /// Index type for a sequence of tokens.
    using TokenSize = std::vector<std::string>::size_type;

    /// Type for representing the current token position.
    ///
    /// Signed type to allow '-1' to represent an invalid position.
    using CurrentPos = std::make_signed_t<TokenSize>;

    /// Sequence of concatenated condition string tokens of a single ACTIONX
    /// condition block.
    ///
    /// Newline characters and other whitespace assume to be removed.
    /// Constructor argument.
    const std::vector<std::string>& tokens_;

    /// Current token position (index into tokens_).
    ///
    /// Default value -1 represents an invalid position.
    CurrentPos current_pos_ = -1;

    /// Retrieve the current string token.
    ///
    /// Will throw an exception of type std::logic_error if the
    /// parseIsComplete().
    ///
    /// \return tokens_[current_pos_].
    [[nodiscard]] const std::string& currentToken() const;

    /// Retrieve the current token type.
    ///
    /// \return Token type of currentToken().  Special 'end' token if the
    /// parseIsComplete().
    [[nodiscard]] Opm::Action::TokenType currentType() const;

    /// Construct a parse tree node from current token.
    [[nodiscard]] Node current() const;

    /// Process completion predicate.
    ///
    /// \return Whether or not we have consumed all input tokens, i.e.,
    /// whether or not the current_pos_ is within the valid range of tokens_
    [[nodiscard]] bool parseIsComplete() const;

    /// Finish current token and advance to next token position.
    void advanceTokenPosition();

    /// Finish current token and advance to next.
    ///
    /// Convenience function which calls advanceTokenPosition() and returns
    /// current() at that position.
    ///
    /// \return Next token.
    [[nodiscard]] Node next();

    /// Parse an arithmetic comparison.
    ///
    /// Current token sequence is expected to be of the form
    ///
    ///    Some Expression <OP> Some Other Expression
    ///
    /// in which <OP> is an arithmetic comparison operator such as '<' or
    /// '.EQ.'.
    ///
    /// This function consumes tokens from tokens_ and advances the current
    /// token position until either the comparison expression is complete,
    /// we encounter a failure condition, or exhaust the token sequence.
    ///
    /// \return Parse tree for an arithmetic comparison.  Will return an
    /// invalid parse tree (TokenType::error) if any of the following
    /// conditions hold
    ///
    ///  * The left or right expressions could not be successfully parsed.
    ///  * The operator <OP> was not recognised as an arithmetic operator.
    [[nodiscard]] Opm::Action::ASTNode parse_cmp();

    /// Parse an arithmetic comparison operator.
    ///
    /// This function consumes a single token from tokens_ and advances the
    /// current token position if successful.  Otherwise, the current token
    /// position is unchanged.
    ///
    /// \return AST node for an arithmetic comparison operator.  Invalid AST
    /// node (TokenType::error) if currentToken() is not recognised as an
    /// arithmetic operator.
    [[nodiscard]] Opm::Action::ASTNode parse_op();

    /// Parse an arithmetic expression to the left of a comparison operator
    ///
    /// This function consumes tokens from tokens_ and advances the current
    /// token position as long as the tokens are expressions or numbers.
    ///
    /// \return AST node for an arithmetic expression.  Invalid AST node
    /// (TokenType::error) if the current token on entry is not an
    /// expression type token.
    [[nodiscard]] Opm::Action::ASTNode parse_left();

    /// Parse an arithmetic expression to the right of a comparison operator
    ///
    /// This function consumes tokens from tokens_ and advances the current
    /// token position as long as the tokens are expressions or numbers.
    ///
    /// \return AST node for an arithmetic expression.  Invalid AST node
    /// (TokenType::error) if the current token on entry is not an
    /// expression type token.
    [[nodiscard]] Opm::Action::ASTNode parse_right();

    /// Parse a sequence of conjunction ('AND') clauses at the same level
    ///
    /// Current token sequence is expected to be of the form
    ///
    ///    Logical_1 (AND Logical_2 AND ... AND Logical_n)_opt
    ///
    /// in which 'Logical_i' are logical expression clauses such as
    /// arithmetic comparisons (parse_cmp()) or some other kind of grouped
    /// logical expressions.  All clauses, including the 'AND' operators,
    /// other than 'Logical_1' are optional.
    ///
    /// This function consumes tokens and advances the current token
    /// position as long as we're able to parse out sub clauses 'Logical_i'.
    ///
    /// \return Parse tree for a conjunction.  Invalid AST node if one of
    /// the sub clauses generates a parse error.
    [[nodiscard]] Opm::Action::ASTNode parse_and();

    /// Parse a sequence of disjunction ('OR') clauses at the same level.
    /// Current token sequence is expected to be of the form
    ///
    ///    Logical_1 (OR Logical_2 OR ... OR Logical_n)_opt
    ///
    /// in which 'Logical_i' are logical expression clauses.  All clauses,
    /// including the 'OR' operators, other than 'Logical_1' are optional.
    ///
    /// This function consumes tokens and advances the current token
    /// position as long as we're able to parse out sub clauses 'Logical_i'.
    ///
    /// \return Parse tree for a disjunction.  Invalid AST node if one of
    /// the sub clauses generates a parse error.
    [[nodiscard]] Opm::Action::ASTNode parse_or();
};

std::unique_ptr<Opm::Action::ASTNode> ActionParser::buildParseTree()
{
    if (const auto start_node = this->next();
        start_node.type == Opm::Action::TokenType::end)
    {
        return std::make_unique<Opm::Action::ASTNode>(start_node.type);
    }

    auto tree = this->parse_or();

    if (tree.type == Opm::Action::TokenType::error) {
        throw std::invalid_argument {
            "Failed to parse ACTIONX condition."
        };
    }

    if (this->currentType() != Opm::Action::TokenType::end) {
        throw std::invalid_argument {
            fmt::format("Extra unhandled data starting with "
                        "token[{}] = {} in ACTIONX condition",
                        this->current_pos_, this->currentToken())
        };
    }

    return std::make_unique<Opm::Action::ASTNode>(std::move(tree));
}

const std::string& ActionParser::currentToken() const
{
    if (this->parseIsComplete()) {
        throw std::logic_error {
            "Parse process proceeds beyond token sequence end"
        };
    }

    return this->tokens_[this->current_pos_];
}

Opm::Action::TokenType ActionParser::currentType() const
{
    return this->parseIsComplete()
        ? Opm::Action::TokenType::end
        : Opm::Action::Parser::tokenType(this->currentToken());
}

ActionParser::Node ActionParser::current() const
{
    if (this->parseIsComplete()) {
        return Node { Opm::Action::TokenType::end };
    }

    const auto& arg = this->currentToken();
    return Node { Opm::Action::Parser::tokenType(arg), arg };
}

bool ActionParser::parseIsComplete() const
{
    assert (this->current_pos_ >= 0);

    return static_cast<TokenSize>(this->current_pos_)
        >= this->tokens_.size();
}

void ActionParser::advanceTokenPosition()
{
    ++this->current_pos_;
}

ActionParser::Node ActionParser::next()
{
    this->advanceTokenPosition();
    return this->current();
}

Opm::Action::ASTNode ActionParser::parse_left()
{
    auto curr = this->current();
    if (curr.type != Opm::Action::TokenType::ecl_expr) {
        throw std::invalid_argument {
            fmt::format("Expected expression as left hand side "
                        "of comparison, but got {} instead.",
                        curr.value)
        };
    }

    // Note: Func must be an independent object here--i.e., not a
    // reference--since we update 'curr' in the loop below.
    const auto func = curr.value;
    const auto func_type = functionType(curr.value);

    auto arg_list = std::vector<std::string>{};

    curr = this->next();
    while ((curr.type == Opm::Action::TokenType::ecl_expr) ||
           (curr.type == Opm::Action::TokenType::number))
    {
        arg_list.push_back(curr.value);
        curr = this->next();
    }

    return Opm::Action::ASTNode {
        Opm::Action::TokenType::ecl_expr, func_type, func, arg_list
    };
}

Opm::Action::ASTNode ActionParser::parse_op()
{
    const auto curr = this->current();

    if ((curr.type == Opm::Action::TokenType::op_gt) ||
        (curr.type == Opm::Action::TokenType::op_ge) ||
        (curr.type == Opm::Action::TokenType::op_lt) ||
        (curr.type == Opm::Action::TokenType::op_le) ||
        (curr.type == Opm::Action::TokenType::op_eq) ||
        (curr.type == Opm::Action::TokenType::op_ne))
    {
        this->advanceTokenPosition();

        return Opm::Action::ASTNode { curr.type };
    }

    return Opm::Action::ASTNode { Opm::Action::TokenType::error };
}

Opm::Action::ASTNode ActionParser::parse_right()
{
    auto curr = this->current();
    if (curr.type == Opm::Action::TokenType::number) {
        this->advanceTokenPosition();

        return Opm::Action::ASTNode {
            std::strtod(curr.value.c_str(), nullptr)
        };
    }

    curr = this->current();
    if (curr.type != Opm::Action::TokenType::ecl_expr) {
        return Opm::Action::ASTNode { Opm::Action::TokenType::error };
    }

    // Note: Func must be an independent object here--i.e., not a
    // reference--since we update 'curr' in the loop below.
    const auto func = curr.value;
    const auto func_type = Opm::Action::FuncType::none;

    auto arg_list = std::vector<std::string>{};

    curr = this->next();
    while ((curr.type == Opm::Action::TokenType::ecl_expr) ||
           (curr.type == Opm::Action::TokenType::number))
    {
        arg_list.push_back(curr.value);
        curr = this->next();
    }

    return Opm::Action::ASTNode {
        Opm::Action::TokenType::ecl_expr, func_type, func, arg_list
    };
}

Opm::Action::ASTNode ActionParser::parse_cmp()
{
    if (this->currentType() == Opm::Action::TokenType::open_paren) {
        this->advanceTokenPosition(); // Consume "("

        const auto inner_expr = this->parse_or();

        if (this->currentType() != Opm::Action::TokenType::close_paren) {
            return Opm::Action::ASTNode { Opm::Action::TokenType::error };
        }

        this->advanceTokenPosition(); // Consume ")"

        return inner_expr;
    }

    auto left_node = this->parse_left();
    if (left_node.type == Opm::Action::TokenType::error) {
        return Opm::Action::ASTNode { Opm::Action::TokenType::error };
    }

    auto op_node = this->parse_op();
    if (op_node.type == Opm::Action::TokenType::error) {
        return Opm::Action::ASTNode { Opm::Action::TokenType::error };
    }

    auto right_node = this->parse_right();
    if (right_node.type == Opm::Action::TokenType::error) {
        return Opm::Action::ASTNode { Opm::Action::TokenType::error };
    }

    op_node.add_child(std::move(left_node));
    op_node.add_child(std::move(right_node));

    return op_node;
}

Opm::Action::ASTNode ActionParser::parse_and()
{
    auto left = this->parse_cmp();
    if (left.type == Opm::Action::TokenType::error) {
        return Opm::Action::ASTNode { Opm::Action::TokenType::error };
    }

    if (this->currentType() != Opm::Action::TokenType::op_and) {
        return left;
    }

    auto and_node = Opm::Action::ASTNode { Opm::Action::TokenType::op_and };
    and_node.add_child(std::move(left));

    while (this->currentType() == Opm::Action::TokenType::op_and) {
        this->advanceTokenPosition();

        auto next_cmp = this->parse_cmp();
        if (next_cmp.type == Opm::Action::TokenType::error) {
            return Opm::Action::ASTNode { Opm::Action::TokenType::error };
        }

        and_node.add_child(std::move(next_cmp));
    }

    return and_node;
}

Opm::Action::ASTNode ActionParser::parse_or()
{
    auto left = this->parse_and();
    if (left.type == Opm::Action::TokenType::error) {
        return Opm::Action::ASTNode { Opm::Action::TokenType::error };
    }

    if (this->currentType() != Opm::Action::TokenType::op_or) {
        return left;
    }

    auto or_node = Opm::Action::ASTNode { Opm::Action::TokenType::op_or };
    or_node.add_child(std::move(left));

    while (this->currentType() == Opm::Action::TokenType::op_or) {
        this->advanceTokenPosition();

        auto next_cmp = this->parse_or();
        if (next_cmp.type == Opm::Action::TokenType::error) {
            return Opm::Action::ASTNode { Opm::Action::TokenType::error };
        }

        or_node.add_child(std::move(next_cmp));
    }

    return or_node;
}

} // Anonymous namespace

namespace Opm::Action::Parser {

std::unique_ptr<ASTNode>
parseCondition(const std::vector<std::string>& tokens)
{
    return ActionParser{tokens}.buildParseTree();
}

TokenType tokenType(const std::string& arg)
{
    const auto lower_arg = makeLowercase(arg);

    if (lower_arg == "and") {
        return TokenType::op_and;
    }

    if (lower_arg == "or") {
        return TokenType::op_or;
    }

    if (lower_arg == "(") {
        return TokenType::open_paren;
    }

    if (lower_arg == ")") {
        return TokenType::close_paren;
    }

    if ((lower_arg == ">") || (lower_arg == ".gt.")) {
        return TokenType::op_gt;
    }

    if ((lower_arg == ">=") || (lower_arg == ".ge.")) {
        return TokenType::op_ge;
    }

    if ((lower_arg == "<") || (lower_arg == ".lt.")) {
        return TokenType::op_lt;
    }

    if ((lower_arg == "<=") || (lower_arg == ".le.")) {
        return TokenType::op_le;
    }

    if ((lower_arg == "=") || (lower_arg == ".eq.")) {
        return TokenType::op_eq;
    }

    if ((lower_arg == "!=") || (lower_arg == ".ne.")) {
        return TokenType::op_ne;
    }

    // Check if 'arg' is a number.
    //
    // Might want to switch to std::from_chars() here, or possibly a
    // different function, one that recognises numbers like 0.123D+4,
    // instead.
    {
        char* end_ptr = nullptr;
        std::strtod(lower_arg.c_str(), &end_ptr);

        if (std::strlen(end_ptr) == 0) {
            return TokenType::number;
        }
    }

    return TokenType::ecl_expr;
}

} // namespace Opm::Action::Parser
