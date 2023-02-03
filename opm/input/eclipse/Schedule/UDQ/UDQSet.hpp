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

#ifndef UDQSET_HPP
#define UDQSET_HPP

#include <opm/input/eclipse/Schedule/UDQ/UDQEnums.hpp>

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

class UDQScalar
{
public:
    UDQScalar() = default;

    /// Constructor
    ///
    /// Forms a UDQ scalar defined by its numeric value and, possibly, a
    /// particular numbered item.
    ///
    /// \param[in] value Numeric value
    /// \param[in] num Item number.
    explicit UDQScalar(const double value, const std::size_t num = 0);

    /// Constructor
    ///
    /// Forms a UDQ scalar attached to a particular name and, possibly, a
    /// particular numbered item.
    ///
    /// \param[in] wgname Named well or group to which this scalar is
    ///    associated.
    ///
    /// \param[in] num Item number.
    explicit UDQScalar(const std::string& wgname, const std::size_t num = 0);

    /// Add other UDQ scalar to this
    ///
    /// Result is defined if both this and other scalar are defined, and the
    /// sum is a finite value.
    ///
    /// \param[in] rhs Other UDQ scalar.
    void operator+=(const UDQScalar& rhs);

    /// Add numeric value to this UDQ scalar.
    ///
    /// Result is defined if \c *this is defined and the sum is a finite
    /// value.
    ///
    /// \param[in] rhs Numeric value.
    void operator+=(double rhs);

    /// Multiply UDQ scalar into this
    ///
    /// Result is defined if both this and other scalar are defined, and if
    /// the product is a finite value.
    ///
    /// \param[in] rhs Other UDQ scalar.
    void operator*=(const UDQScalar& rhs);

    /// Multiply numeric value into this
    ///
    /// Result is defined if \c *this is defined and the product is a finite
    /// value.
    ///
    /// \param[in] rhs Numeric value.
    void operator*=(double rhs);

    /// Divide this UDQ scalar by other
    ///
    /// Result is defined if both this and other scalar are defined, and the
    /// quotient is a finite value.
    ///
    /// \param[in] rhs Other UDQ scalar.
    void operator/=(const UDQScalar& rhs);

    /// Divide this UDQ scalar by numeric value
    ///
    /// Result is defined if \c *this is defined and the quotient is a
    /// finite value.
    ///
    /// \param[in] rhs Numeric value.
    void operator/=(double rhs);

    /// Subtract other UDQ scalar from this.
    ///
    /// Result is defined if both this and other scalar are defined, and if
    /// the difference is a finite value.
    ///
    /// \param[in] rhs Other UDQ scalar.
    void operator-=(const UDQScalar& rhs);

    /// Add other UDQ scalar to this
    ///
    /// Result is defined if \c *this is defined and the difference is a
    /// finite value.
    ///
    /// \param[in] rhs Numeric value.
    void operator-=(double rhs);

    /// Predicate for whether or not this UDQ scalar has a defined value
    ///
    /// \returns \code this->defined() \endcode.
    operator bool() const;

    /// Assign numeric value to this UDQ scalar.
    ///
    /// \param[in] value Numeric value.  Empty optional or non-finite value
    ///    makes this UDQ scalar undefined.
    void assign(const std::optional<double>& value);

    /// Assign numeric value to this UDQ scalar.
    ///
    /// \param[in] value Numeric value.  Non-finite value makes this UDQ
    ///    scalar undefined.
    void assign(double value);

    /// Predicate for whether or not this UDQ scalar has a defined value.
    bool defined() const;

    /// Retrive contained numeric value.
    ///
    /// Throws an exception unless this UDQ scalar has a defined value.
    double get() const;

    /// Retrive contained numeric value.
    ///
    /// Empty optional unless this scalar has a defined value.
    const std::optional<double>& value() const { return this->m_value; }

    /// Retrive named well/group to which this scalar is associated.
    const std::string& wgname() const { return this->m_wgname; }

    /// Retrive numbered item, typically segment or connection, to which
    /// this scalar is associated.
    ///
    /// Always zero for non-numbered UDQ scalars.
    std::size_t number() const { return this->m_num; }

    /// Equality predicate
    ///
    /// \param[in] UDQ scalar to which this scalar will be compared for
    ///    equality.
    bool operator==(const UDQScalar& other) const;

public:
    /// Scalar value.
    std::optional<double> m_value{};

    /// Associated well/group name
    std::string m_wgname{};

    /// Numbered item.  Typically segment or connection.  Zero for
    /// non-numbered items.
    std::size_t m_num = 0;
};


class UDQSet
{
public:
    // Connections and segments.
    struct EnumeratedWellItems {
        std::string well{};
        std::vector<std::size_t> numbers{};

        bool operator==(const EnumeratedWellItems& rhs) const;
        static EnumeratedWellItems serializationTestObject();

        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->well);
            serializer(this->numbers);
        }
    };

    /// Construct empty, named UDQ set of specific variable type
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] var_type UDQ set's variable type.
    UDQSet(const std::string& name, UDQVarType var_type);

    /// Construct named UDQ set of specific variable type for particular set
    /// of well/group names.
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] var_type UDQ set's variable type.
    ///
    /// \param[in] wgnames Well or group names.
    UDQSet(const std::string& name, UDQVarType var_type, const std::vector<std::string>& wgnames);

    /// Construct named UDQ set of specific variable type for numbered well
    /// items-typically segments or connections.
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] var_type UDQ set's variable type.
    ///
    /// \param[in] items Enumerated entities for which this UDQ set is
    ///    defined.  Typically represents a set of segments in an MS well or
    ///    a set of well/reservoir connections.
    UDQSet(const std::string& name, UDQVarType var_type, const std::vector<EnumeratedWellItems>& items);

    /// Construct empty, named UDQ set of specific variable type
    ///
    /// \param[in] name UDQ set name
    /// \param[in] var_type UDQ set's variable type.
    UDQSet(const std::string& name, UDQVarType var_type, std::size_t size);

    /// Construct empty, named UDQ set of specific variable type
    ///
    /// \param[in] name UDQ set name
    /// \param[in] var_type UDQ set's variable type.
    UDQSet(const std::string& name, std::size_t size);

    /// Form a UDQ set pertaining to a single scalar value.
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] scalar_value Initial numeric value of this scalar set.
    ///    Empty optional or non-finite contained value leaves the UDQ set
    ///    undefined.
    static UDQSet scalar(const std::string& name, const std::optional<double>& scalar_value);

    /// Form a UDQ set pertaining to a single scalar value.
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] scalar_value Initial numeric value of this scalar set.
    ///    Non-finite value leaves the UDQ set undefined.
    static UDQSet scalar(const std::string& name, double value);

    /// Form an empty UDQ set.
    ///
    /// \param[in] name UDQ set name
    static UDQSet empty(const std::string& name);

    /// Form a UDQ set pertaining to a set of named wells
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] wells Collection of named wells.
    static UDQSet wells(const std::string& name, const std::vector<std::string>& wells);

    /// Form a UDQ set pertaining to a set of named wells
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] wells Collection of named wells.
    ///
    /// \param[in] scalar_value Initial numeric value of every element of
    ///    this UDQ set.  Non-finite value leaves the UDQ set elements
    ///    undefined.
    static UDQSet wells(const std::string& name, const std::vector<std::string>& wells, double scalar_value);

    /// Form a UDQ set pertaining to a set of named groups
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] wells Collection of named groups.
    static UDQSet groups(const std::string& name, const std::vector<std::string>& groups);

    /// Form a UDQ set pertaining to a set of named groups
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] groups Collection of named groups.
    ///
    /// \param[in] scalar_value Initial numeric value of every element of
    ///    this UDQ set.  Non-finite value leaves the UDQ set elements
    ///    undefined.
    static UDQSet groups(const std::string& name, const std::vector<std::string>& groups, double scalar_value);

    /// Form a UDQ set at the field level
    ///
    /// \param[in] name UDQ set name
    ///
    /// \param[in] scalar_value Initial numeric value of every element of
    ///    this UDQ set.  Non-finite value leaves the UDQ set element
    ///    undefined.
    static UDQSet field(const std::string& name, double scalar_value);

    /// Assign value to every element of the UDQ set
    ///
    /// \param[in] value Numeric value for each element.  Empty optional
    ///    removes element from UDQ set.
    void assign(const std::optional<double>& value);

    /// Assign value to specific element of the UDQ set
    ///
    /// \param[in] index Linear index into UDQ set.  Must be in the range
    ///    0..size()-1 inclusive.
    ///
    /// \param[in] value Numeric value of specific UDQ set element.  Empty
    ///    optional removes element from UDQ set.
    void assign(std::size_t index, const std::optional<double>& value);

    /// Assign value to specific element of the UDQ set
    ///
    /// \param[in] wgname Named element of UDQ set defined for wells/groups.
    ///
    /// \param[in] value Numeric value of specific UDQ set element.  Empty
    ///    optional removes element from UDQ set.
    void assign(const std::string& wgname, const std::optional<double>& value);

    /// Assign value to specific element of the UDQ set
    ///
    /// \param[in] wgname Named element of UDQ set defined for wells.
    ///
    /// \param[in] number Numbered sub-element of named UDQ set element
    ///    referred to by \p wgname.
    ///
    /// \param[in] value Numeric value of specific UDQ set element.  Empty
    ///    optional removes element from UDQ set.
    void assign(const std::string& wgname, std::size_t number, const std::optional<double>& value);

    /// Assign value to every element of the UDQ set
    ///
    /// \param[in] value Numeric value for each element.  Non-finite numeric
    ///    value removes element from UDQ set.
    void assign(double value);

    /// Assign value to specific element of the UDQ set
    ///
    /// \param[in] index Linear index into UDQ set.  Must be in the range
    ///    0..size()-1 inclusive.
    ///
    /// \param[in] value Numeric value of specific UDQ set element.
    ///    Non-finite numeric value removes element from UDQ set.
    void assign(std::size_t index, double value);

    /// Assign value to specific element of the UDQ set
    ///
    /// \param[in] wgname Named element of UDQ set defined for wells/groups.
    ///
    /// \param[in] value Numeric value of specific UDQ set element.
    ///    Non-finite numeric value removes element from UDQ set.
    void assign(const std::string& wgname, double value);

    /// Predicate for whether or not named UDQ element exists.
    bool has(const std::string& name) const;

    /// Number of elements in UDQ set.  Undefined elements counted the same
    /// as defined elements.
    std::size_t size() const;

    /// Add other UDQ set into this
    ///
    /// Defined elements in result is intersection of defined elements in \c
    /// *this and \p rhs.
    ///
    /// \param[in] rhs Other UDQ set.
    void operator+=(const UDQSet& rhs);

    /// Add numeric value to all defined elements of this UDQ set.
    ///
    /// \param[in] rhs Numeric value.
    void operator+=(double rhs);

    /// Subtract UDQ set from this
    ///
    /// Defined elements in result is intersection of defined elements in \c
    /// *this and \p rhs.
    ///
    /// \param[in] rhs Other UDQ set.
    void operator-=(const UDQSet& rhs);

    /// Subtract numeric values from all defined elements of this UDQ set.
    ///
    /// \param[in] rhs Numeric value.
    void operator-=(double rhs);

    /// Multiply other UDQ set into this.
    ///
    /// Defined elements in result is intersection of defined elements in \c
    /// *this and \p rhs.  Multiplication is performed per element.
    ///
    /// \param[in] rhs Other UDQ set.
    void operator*=(const UDQSet& rhs);

    /// Multiply every defined element with numeric element
    ///
    /// \param[in] rhs Numeric value.
    void operator*=(double rhs);

    /// Divide this UDQ set by other UDQ set.
    ///
    /// Defined elements in result is intersection of defined elements in \c
    /// *this and \p rhs.  Division is performed per element.
    ///
    /// \param[in] rhs Other UDQ set.
    void operator/=(const UDQSet& rhs);

    /// Divide every defined element with numeric element
    ///
    /// \param[in] rhs Numeric value.
    void operator/=(double rhs);

    /// Access individual UDQ scalar at particular index in UDQ set.
    ///
    /// \param[in] index Linear index into UDQ set.  Must be in the range
    ///    0..size()-1 inclusive.  Indexing operator throws an exception if
    ///    the index is out of bounds.
    const UDQScalar& operator[](std::size_t index) const;

    /// Access individual UDQ scalar assiociated to particular named entity
    /// (well or group).
    ///
    /// \param[in] wgname Named entity.  Indexing operator throws an
    ///    exception if no element exists for this named entity.
    const UDQScalar& operator[](const std::string& wgname) const;

    /// Access individual UDQ scalar assiociated to particular named well
    /// and numbered sub-entity of that named well.
    ///
    /// \param[in] well Well name.  Indexing operator throws an
    ///    exception if no element exists for this named well.
    ///
    /// \param[in] item Numbered sub-entity of named well.  Typically a
    ///    segment or connection number.  Indexing operator throws an
    ///    exception if no element exists for the numbered sub-entity of
    ///    this well.
    const UDQScalar& operator()(const std::string& well, const std::size_t item) const;

    /// Range-for traversal support (beginning of range)
    std::vector<UDQScalar>::const_iterator begin() const;

    /// Range-for traversal support (one past end of range)
    std::vector<UDQScalar>::const_iterator end() const;

    /// Retrive names of entities associate to this UDQ set.
    std::vector<std::string> wgnames() const;

    /// Retrive the UDQ set's defined values only
    std::vector<double> defined_values() const;

    /// Retrive the UDQ set's number of defined values
    std::size_t defined_size() const;

    /// Retrive the name of this UDQ set
    const std::string& name() const;

    /// Assign the name of this UDQ set
    void name(const std::string& name);

    /// Retrive the variable type of this UDQ set (e.g., well, group, field,
    /// segment &c).
    UDQVarType var_type() const;

    /// Equality comparison operator.
    ///
    /// \param[in] other UDQ set to which this set will be compared for equality.
    bool operator==(const UDQSet& other) const;

private:
    /// UDQ set name
    std::string m_name;

    /// UDQ set's variable type
    UDQVarType m_var_type = UDQVarType::NONE;

    /// UDQ set's element values.
    std::vector<UDQScalar> values;

    /// Default constructor.  For implementing the named constructors only.
    UDQSet() = default;
};


UDQScalar operator+(const UDQScalar& lhs, const UDQScalar& rhs);
UDQScalar operator+(const UDQScalar& lhs, double rhs);
UDQScalar operator+(double lhs, const UDQScalar& rhs);

UDQScalar operator-(const UDQScalar& lhs, const UDQScalar& rhs);
UDQScalar operator-(const UDQScalar& lhs, double rhs);
UDQScalar operator-(double lhs, const UDQScalar& rhs);

UDQScalar operator*(const UDQScalar& lhs, const UDQScalar& rhs);
UDQScalar operator*(const UDQScalar& lhs, double rhs);
UDQScalar operator*(double lhs, const UDQScalar& rhs);

UDQScalar operator/(const UDQScalar& lhs, const UDQScalar& rhs);
UDQScalar operator/(const UDQScalar& lhs, double rhs);
UDQScalar operator/(double lhs, const UDQScalar& rhs);

UDQSet operator+(const UDQSet& lhs, const UDQSet& rhs);
UDQSet operator+(const UDQSet& lhs, double rhs);
UDQSet operator+(double lhs, const UDQSet& rhs);

UDQSet operator-(const UDQSet& lhs, const UDQSet& rhs);
UDQSet operator-(const UDQSet& lhs, double rhs);
UDQSet operator-(double lhs, const UDQSet& rhs);

UDQSet operator*(const UDQSet& lhs, const UDQSet& rhs);
UDQSet operator*(const UDQSet& lhs, double rhs);
UDQSet operator*(double lhs, const UDQSet& rhs);

UDQSet operator/(const UDQSet& lhs, const UDQSet& rhs);
UDQSet operator/(const UDQSet& lhs, double rhs);
UDQSet operator/(double lhs, const UDQSet&rhs);

} // namespace Opm

#endif  // UDQSET_HPP
