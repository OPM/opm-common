/*
  Copyright 2020 Equinor ASA.

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

#ifndef RPT_CONFIG_HPP
#define RPT_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Opm {

    class DeckKeyword;
    class ParseContext;
    class ErrorGuard;

} // namespace Opm

namespace Opm {

/// Configuration manager for RPTSCHED and RPTSOL keywords.
class RPTConfig
{
public:
    /// Default constructor.
    ///
    /// Creates a configuration object that's mostly usable as a target for
    /// a deserialisation operation.
    RPTConfig() = default;

    /// Constructor.
    ///
    /// Internalises and normalises the specification of an RPTSOL keyword
    /// into a set of mnemonics and associate values.  Supports both regular
    /// mnemonics and integer controls.  Performs no error checking and
    /// therefore accepts all mnemonics.
    ///
    /// \param[in] keyword RPTSCHED keyword specification.
    explicit RPTConfig(const DeckKeyword& keyword);

    /// Constructor.
    ///
    /// Internalises and normalises the specification of an RPTSCHED keyword
    /// into a set of mnemonics and associate values.  Checks the input
    /// specification against a known set of mnemonics and rejects unknown
    /// mnemonics.  Expands an existing set of mnemonics if provided as
    /// input.  Supports both regular mnemonics and integer controls.
    ///
    /// \param[in] keyword RPTSCHED keyword specification.
    ///
    /// \param[in] prev Existing set of mnemonics that will potentially be
    /// expanded by the requests in \p keyword.  Pass \c nullptr if there is
    /// no existing set of mnemonics.
    ///
    /// \param[in] parseContext Error handling controls.
    ///
    /// \param[in,out] errors Collection of parse errors encountered thus
    /// far.  Behaviour controlled by \p parseContext.
    explicit RPTConfig(const DeckKeyword&  keyword,
                       const RPTConfig*    prev,
                       const ParseContext& parseContext,
                       ErrorGuard&         errors);

    /// Mnemonic existence predicate.
    ///
    /// Queries the internal, normalised collection, for whether or not a
    /// particular mnemonic exists.
    ///
    /// \param[in] key Mnemonic string.
    ///
    /// \returns Whether or not \p key exists in the internal collection.
    bool contains(const std::string& key) const;

    /// Start of internal mnemonic sequence.
    ///
    /// To support iteration.
    auto begin() const { return this->mnemonics_.begin(); }

    /// End of internal mnemonic sequence.
    ///
    /// To support iteration.
    auto end() const { return this->mnemonics_.end(); }

    /// Number of mnemonics in internal sequence.
    auto size() const { return this->mnemonics_.size(); }

    /// Get read/write access to particular mnemonic value.
    ///
    /// Will throw an exception if the mnemonic does not exist in the
    /// internal collection.  Use predicate contains() to check existence.
    ///
    /// \param[in] key Mnemonic string.
    ///
    /// \return Reference to mutable mnemonic value for \p key.
    unsigned& at(const std::string& key) { return this->mnemonics_.at(key); }

    /// Get read-only access to particular mnemonic value.
    ///
    /// Will throw an exception if the mnemonic does not exist in the
    /// internal collection.  Use predicate contains() to check existence.
    ///
    /// \param[in] key Mnemonic string.
    ///
    /// \return Mnemonic value for \p key.
    unsigned at(const std::string& key) const { return this->mnemonics_.at(key); }

    /// Equality predicate.
    ///
    /// \param[in] other Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p other.
    bool operator==(const RPTConfig& other) const;

    /// Create a serialisation test object.
    static RPTConfig serializationTestObject();

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->mnemonics_);
    }

private:
    /// Collection of RPTSCHED mnemonics and their associate values.
    std::unordered_map<std::string, unsigned int> mnemonics_{};

    /// Assign mnemonic values from RPTSCHED keyword.
    ///
    /// \param[in] mnemonics List of mnemonics and associate values.
    /// Typically from normalising an RPTSCHED specification.
    void assignMnemonics(const std::vector<std::pair<std::string, int>>& mnemonics);
};

} // namespace Opm

#endif // RPT_CONFIG_HPP
