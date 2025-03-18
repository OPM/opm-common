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

#include <map>
#include <string>
#include <unordered_map>

namespace Opm {

    class DeckKeyword;
    class ParseContext;
    class ErrorGuard;

} // namespace Opm

namespace Opm {

class RPTConfig
{
public:
    RPTConfig() = default;
    explicit RPTConfig(const DeckKeyword& keyword);

    explicit RPTConfig(const DeckKeyword&  keyword,
                       const ParseContext& parseContext,
                       ErrorGuard&         errors,
                       const RPTConfig*    prev);

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_mnemonics);
    }

    bool contains(const std::string& key) const;

    auto begin() const { return this->m_mnemonics.begin(); };
    auto end() const { return this->m_mnemonics.end(); };
    auto size() const { return this->m_mnemonics.size(); };

    unsigned& at(const std::string& key) { return this->m_mnemonics.at(key); };
    unsigned at(const std::string& key) const { return this->m_mnemonics.at(key); };

    static RPTConfig serializationTestObject();
    bool operator==(const RPTConfig& other) const;

private:
    std::unordered_map<std::string, unsigned int> m_mnemonics{};

    void assignMnemonics(const std::map<std::string, int>& mnemonics);
};

} // namespace Opm

#endif // RPT_CONFIG_HPP
