/*
  Copyright (C) 2024 Equinor

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
#ifndef OPM_PARSER_EZROKHI_TABLE_HPP
#define OPM_PARSER_EZROKHI_TABLE_HPP

#include <cstddef>
#include <unordered_map>

namespace Opm {
class DeckRecord;

struct EzrokhiRecord {
    double c0;
    double c1;
    double c2;

    EzrokhiRecord() = default;
    EzrokhiRecord(double c0_in, double c1_in, double c2_in) :
        c0(c0_in),
        c1(c1_in),
        c2(c2_in)
    {};

    bool operator==(const EzrokhiRecord& other) const {
        return this->c0 == other.c0 &&
               this->c1 == other.c1 &&
               this->c2 == other.c2;
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(c0);
        serializer(c2);
        serializer(c2);
    }

};

class EzrokhiTable {
public:
    EzrokhiTable() = default;
    explicit EzrokhiTable( std::unordered_map<std::string, EzrokhiRecord> records);

    static EzrokhiTable serializationTestObject();

    void init(const DeckRecord& record, const std::string cname, const int icomp);
    std::size_t size() const;
    bool empty() const;
    std::unordered_map<std::string, EzrokhiRecord>::const_iterator begin() const;
    std::unordered_map<std::string, EzrokhiRecord>::const_iterator end() const;
    const EzrokhiRecord& operator[](const std::string name) const;

    double getC0(const std::string name) const;
    double getC1(const std::string name) const;
    double getC2(const std::string name) const;

    bool operator==(const EzrokhiTable& other) const {
        return this->data == other.data;
    }

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(data);
    }

private:
    std::unordered_map<std::string, EzrokhiRecord> data;
};

}  // namespace Opm

#endif