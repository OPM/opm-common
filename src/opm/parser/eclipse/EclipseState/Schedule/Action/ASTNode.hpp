#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionContext.hpp>

#include "ActionValue.hpp"

namespace Opm {

class ActionContext;
class WellSet;
class ASTNode {
public:

    ASTNode();
    ASTNode(TokenType type_arg);
    ASTNode(double value);
    ASTNode(TokenType type_arg, const std::string& func_arg, const std::vector<std::string>& arg_list_arg);

    bool eval(const ActionContext& context, WellSet& matching_wells) const;
    ActionValue value(const ActionContext& context) const;
    TokenType type;
    void add_child(const ASTNode& child);
    size_t size() const;

private:
    std::string func;
    std::vector<std::string> arg_list;
    double number = 0.0;

    /*
      To have a member std::vector<ASTNode> inside the ASTNode class is
      supposedly borderline undefined behaviour; it compiles without warnings
      and works. Good for enough for me.
    */
    std::vector<ASTNode> children;
};

}
#endif
