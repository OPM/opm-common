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

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace Opm::Action {

class Context;
class ASTNode;

} // namespace Opm::Action

namespace Opm::Action {

/// Expression evaluation tree of a full ACTIONX condition block.
///
/// There is no additional context such as current summary vector values or
/// a set of active wells.  This must be supplied through an Action::Context
/// instace when invoking the eval() member function.

class AST
{
public:
    /// Default constructor.
    ///
    /// Creates an empty object with no internal condition.  Mostly useful
    /// as a target for an object deserialisation process.
    AST();

    /// Constructor.
    ///
    /// Forms the internal expression tree by parsing a sequence of text
    /// tokens.
    ///
    /// \param[in] tokens Tokenised textual representation of an ACTIONX
    /// condition block.
    explicit AST(const std::vector<std::string>& tokens);

    /// Destructor.
    ~AST();

    /// Copy constructor.
    ///
    /// \param[in] rhs Source object.
    AST(const AST& rhs);

    /// Move constructor.
    ///
    /// \param[in,out] rhs Source object.  In a valid but unspecified state
    /// on exit.
    AST(AST&& rhs);

    /// Assignment operator.
    ///
    /// \param[in] rhs Source object.
    ///
    /// \return \code *this \endcode
    AST& operator=(const AST& rhs);

    /// Move assignment operator.
    ///
    /// \param[in,out] rhs Source object.  In a valid but unspecified state
    /// on exit.
    ///
    /// \return \code *this \endcode
    AST& operator=(AST&& rhs);

    /// Create a serialisation test object.
    static AST serializationTestObject();

    /// Evaluate the expression tree at current dynamic state.
    ///
    /// \param[in] context Current summary vectors and wells
    ///
    /// \return Condition value.  A 'true' result means the condition is
    /// satisfied while a 'false' result means the condition is not
    /// satisfied.  Any wells for which the expression is true will be
    /// included in the result set.  A 'false' result has no matching wells.
    Result eval(const Context& context) const;

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will
    /// be tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const AST& data) const;

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(condition);
    }

    /// Export all summary vectors needed to evaluate the expression tree.
    ///
    /// \param[in,out] required_summary Named summary vectors.  Upon
    /// completion, any additional summary vectors needed to evaluate the
    /// full condition block of the current AST object will be included in
    /// this set.
    void required_summary(std::unordered_set<std::string>& required_summary) const;

private:
    /// Internalised condition object in expression tree form.
    std::unique_ptr<ASTNode> condition{};
};

} // namespace Opm::Action

#endif // ActionAST_HPP
