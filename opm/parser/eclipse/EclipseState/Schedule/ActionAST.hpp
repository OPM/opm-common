/*
  Copyright 2018 Equinor ASA.

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


#ifndef ActionAST_HPP
#define ActionAST_HPP

#include <string>
#include <vector>


namespace Opm {

class ActionContext;

enum TokenType {
    number,        //  0
    ecl_expr,      //  1
    open_paren,    //  2
    close_paren,   //  3
    op_gt,         //  4
    op_ge,         //  5
    op_lt,         //  6
    op_le,         //  7
    op_eq,         //  8
    op_ne,         //  9
    op_and,        // 10
    op_or,         // 11
    end,           // 12
    error          // 13
};

struct ParseNode {
    ParseNode(TokenType type, const std::string& value) :
        type(type),
        value(value)
    {}


    ParseNode(TokenType type) : ParseNode(type, "")
    {}


    TokenType type;
    std::string value;
};


class ASTNode {
public:

ASTNode() :
    type(TokenType::error)
{}


ASTNode(TokenType type):
    type(type)
{}


ASTNode(double value) :
    type(TokenType::number),
    number(value)
{}


ASTNode(TokenType type, const std::string& func, const std::vector<std::string>& arg_list):
    type(type),
    func(func),
    arg_list(arg_list)
{}

    bool eval(const ActionContext& context) const;
    double value(const ActionContext& context) const;
    TokenType type;
    void add_child(const ASTNode& child);
    size_t size() const;

private:
    std::string func;
    std::vector<std::string> arg_list;
    double number;

    /*
      To have a memmber std::vector<ASTNode> inside the ASTNode class is
      supposedly borderline undefined behaviour; it compiles without warnings
      and works. Good for enough for me.
    */
    std::vector<ASTNode> children;
};



class ActionParser {
public:
    ActionParser(const std::vector<std::string>& tokens);
    TokenType get_type(const std::string& arg) const;
    ParseNode current() const;
    ParseNode next();
    size_t pos() const;
    void print() const;
private:
    const std::vector<std::string>& tokens;
    ssize_t current_pos = -1;
};


class ActionAST{
public:
    ActionAST() = default;
    explicit ActionAST(const std::vector<std::string>& tokens);
    ASTNode parse_right(ActionParser& parser);
    ASTNode parse_left(ActionParser& parser);
    ASTNode parse_op(ActionParser& parser);
    ASTNode parse_cmp(ActionParser& parser);
    ASTNode parse_or(ActionParser& parser);
    ASTNode parse_and(ActionParser& parser);

    bool eval(const ActionContext& context) const;
private:
    ASTNode tree;
};
}
#endif
