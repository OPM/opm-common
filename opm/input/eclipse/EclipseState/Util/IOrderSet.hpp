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

#ifndef OPM_IORDER_SET_HPP
#define OPM_IORDER_SET_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace Opm {

/// Set of elements which preserves order of element insertion.
///
/// Repeated insertion of a particular element leaves container unchanged.
///
/// \tparam T Element type.  Should typically be a fairly small type, such
/// as a built-in arithmetic type or a std::string.
template <typename T>
class IOrderSet
{
public:
    /// Default constructor.
    ///
    /// Resulting object is usable as target for a deserialisation
    /// operation, and population through insert()/erase().
    IOrderSet() = default;

    /// Constructor.
    ///
    /// Populates container with an initial set of elements.
    ///
    /// \param[in] data Ordered view of element collection.
    explicit IOrderSet(const std::vector<T>& data)
        : index_ { data.begin(), data.end() }
        , data_  { data }
    {
        if (this->data_.size() != this->index_.size()) {
            throw std::invalid_argument {
                "Initial sequence has duplicate elements"
            };
        }
    }

    /// Number of elements in collection.
    auto size() const
    {
        return this->index_.size();
    }

    /// Whether or not this collection is empty.
    auto empty() const
    {
        return this->size() == 0;
    }

    /// Whether or not a particular element exists in the collection.
    ///
    /// \param[in] value Element.
    ///
    /// \return Whether or no \p value exists in the collection.
    auto contains(const T& value) const
    {
        return this->index_.find(value) != this->index_.end();
    }

    /// Insert element into collection.
    ///
    /// If element already exists, then collection is unchanged.  Otherwise,
    /// the element will be appended to the ordered view of the collection's
    /// elements.
    ///
    /// \param[in] value Element.
    ///
    /// \return Whether or not the \p value was inserted into the
    /// collection.
    bool insert(const T& value)
    {
        const auto& stat = this->index_.insert(value);

        if (stat.second) {
            this->data_.push_back(value);
        }

        return stat.second;
    }

    /// Remove element from collection
    ///
    /// If element does not exist in the collection, the collection is
    /// unchanged.
    ///
    /// \param[in] value Element.
    ///
    /// \return Number of elements removed from collection (0 or 1).
    std::size_t erase(const T& value)
    {
        if (!this->contains(value)) {
            return 0;
        }

        this->index_.erase(value);

        auto data_iter = std::find(this->data_.begin(), this->data_.end(), value);
        this->data_.erase(data_iter);

        return 1;
    }

    /// Iterator to first element in ordered collection view.
    auto begin() const { return this->data_.begin(); }

    /// End of ordered collection view.
    auto end() const { return this->data_.end(); }

    /// Access element by index in ordered collection view.
    ///
    /// Throws an exception of type std::out_of_range if the index is not
    /// strictly less than size().
    ///
    /// \param[in] i Element index.  Should be strictly less than size().
    ///
    /// \return Element at position \p i in ordered collection view.
    const T& operator[](const std::size_t i) const
    {
        return this->data_.at(i);
    }

    /// Ordered collection view.
    const std::vector<T>& data() const
    {
        return this->data_;
    }

    /// Equality predicate.
    ///
    /// \param[in] data Object against which \code *this \endcode will be
    /// tested for equality.
    ///
    /// \return Whether or not \code *this \endcode is the same as \p data.
    bool operator==(const IOrderSet<T>& data) const
    {
        return (this->index_ == data.index_)
            && (this->data_ == data.data_);
    }

    /// Convert between byte array and object representation.
    ///
    /// \tparam Serializer Byte array conversion protocol.
    ///
    /// \param[in,out] serializer Byte array conversion object.
    template <class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->index_);
        serializer(this->data_);
    }

private:
    /// Unordered collection view.
    std::unordered_set<T> index_{};

    /// Ordered collection view.
    std::vector<T> data_{};
};

} // namespace Opm

#endif // OPM_IORDER_SET_HPP
